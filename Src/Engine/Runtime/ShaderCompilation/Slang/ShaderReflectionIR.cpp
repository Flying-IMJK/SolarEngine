#include "ShaderReflectionIR.h"

namespace SE
{
	namespace
	{
		uint32 RotateRight(const uint32 value, const uint32 count)
		{
			return (value >> count) | (value << (32 - count));
		}

		void TransformSHA256(uint32 state[8], const byte block[64])
		{
			static const uint32 constants[64] =
			{
				0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
				0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
				0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
				0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
				0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
				0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
				0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
				0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
			};

			uint32 words[64];
			for (uint32 index = 0; index < 16; index++)
			{
				const uint32 offset = index * 4;
				words[index] = (static_cast<uint32>(block[offset]) << 24) | (static_cast<uint32>(block[offset + 1]) << 16) | (static_cast<uint32>(block[offset + 2]) << 8) | block[offset + 3];
			}
			for (uint32 index = 16; index < 64; index++)
			{
				const uint32 s0 = RotateRight(words[index - 15], 7) ^ RotateRight(words[index - 15], 18) ^ (words[index - 15] >> 3);
				const uint32 s1 = RotateRight(words[index - 2], 17) ^ RotateRight(words[index - 2], 19) ^ (words[index - 2] >> 10);
				words[index] = words[index - 16] + s0 + words[index - 7] + s1;
			}

			uint32 a = state[0];
			uint32 b = state[1];
			uint32 c = state[2];
			uint32 d = state[3];
			uint32 e = state[4];
			uint32 f = state[5];
			uint32 g = state[6];
			uint32 h = state[7];
			for (uint32 index = 0; index < 64; index++)
			{
				const uint32 s1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
				const uint32 choice = (e & f) ^ ((~e) & g);
				const uint32 temp1 = h + s1 + choice + constants[index] + words[index];
				const uint32 s0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
				const uint32 majority = (a & b) ^ (a & c) ^ (b & c);
				const uint32 temp2 = s0 + majority;
				h = g;
				g = f;
				f = e;
				e = d + temp1;
				d = c;
				c = b;
				b = a;
				a = temp1 + temp2;
			}
			state[0] += a;
			state[1] += b;
			state[2] += c;
			state[3] += d;
			state[4] += e;
			state[5] += f;
			state[6] += g;
			state[7] += h;
		}

		void AppendDescriptorText(String& text, const ShaderIRDescriptorRange& range)
		{
			text += String::Format(SE_TEXT("{0}|{1}|{2}|{3}|{4}|{5}|{6}|{7}|"), range.Set, range.Binding, range.Role, range.DescriptorType, range.DescriptorCount, range.ArrayElementBase, range.LogicalElementStride, range.StageMask);
			for (int32 flagIndex = 0; flagIndex < range.Flags.Count(); flagIndex++)
			{
				text += range.Flags[flagIndex];
				text += SE_TEXT(",");
			}
			text += SE_TEXT("\n");
		}
	}

	String ComputeSHA256Hex(const byte* data, const int32 length)
	{
		if (data == nullptr || length < 0)
		{
			return String::Empty;
		}

		uint32 state[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
		int32 offset = 0;
		while (offset + 64 <= length)
		{
			TransformSHA256(state, data + offset);
			offset += 64;
		}

		byte tail[128] = {};
		const int32 tailLength = length - offset;
		for (int32 index = 0; index < tailLength; index++)
		{
			tail[index] = data[offset + index];
		}
		tail[tailLength] = 0x80;
		const uint64 bitLength = static_cast<uint64>(length) * 8;
		const int32 paddedLength = tailLength + 1 <= 56 ? 64 : 128;
		for (int32 index = 0; index < 8; index++)
		{
			tail[paddedLength - 1 - index] = static_cast<byte>(bitLength >> (index * 8));
		}
		TransformSHA256(state, tail);
		if (paddedLength == 128)
		{
			TransformSHA256(state, tail + 64);
		}

		Char output[65];
		static const Char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
		for (int32 stateIndex = 0; stateIndex < 8; stateIndex++)
		{
			for (int32 byteIndex = 0; byteIndex < 4; byteIndex++)
			{
				const byte value = static_cast<byte>(state[stateIndex] >> ((3 - byteIndex) * 8));
				output[(stateIndex * 4 + byteIndex) * 2] = hex[value >> 4];
				output[(stateIndex * 4 + byteIndex) * 2 + 1] = hex[value & 0x0f];
			}
		}
		output[64] = 0;
		return String(output, 64);
	}

	String BuildPipelineLayoutFingerprint(const ShaderReflectionIR& layout)
	{
		List<ShaderIRDescriptorRange> descriptors;
		for (int32 blockIndex = 0; blockIndex < layout.ParameterBlocks.Count(); blockIndex++)
		{
			const ShaderIRParameterBlock& block = layout.ParameterBlocks[blockIndex];
			if (block.HasDefaultUniformBuffer)
			{
				descriptors.Add(block.DefaultUniformBuffer);
			}
			for (int32 bindingIndex = 0; bindingIndex < block.RangeBindings.Count(); bindingIndex++)
			{
				const ShaderIRRangeBinding& binding = block.RangeBindings[bindingIndex];
				for (int32 descriptorIndex = 0; descriptorIndex < binding.DescriptorRanges.Count(); descriptorIndex++)
				{
					descriptors.Add(binding.DescriptorRanges[descriptorIndex]);
				}
			}
		}
		for (int32 left = 0; left < descriptors.Count(); left++)
		{
			for (int32 right = left + 1; right < descriptors.Count(); right++)
			{
				if (descriptors[right].Set < descriptors[left].Set || (descriptors[right].Set == descriptors[left].Set && descriptors[right].Binding < descriptors[left].Binding))
				{
					ShaderIRDescriptorRange value = descriptors[left];
					descriptors[left] = descriptors[right];
					descriptors[right] = value;
				}
			}
		}

		String canonical = SE_TEXT("SOLAR-PIPELINE-LAYOUT/v1\n");
		for (int32 descriptorIndex = 0; descriptorIndex < descriptors.Count(); descriptorIndex++)
		{
			AppendDescriptorText(canonical, descriptors[descriptorIndex]);
		}
		const StringAnsi ansi(canonical);
		return ComputeSHA256Hex(reinterpret_cast<const byte*>(ansi.Get()), ansi.Length());
	}
}
