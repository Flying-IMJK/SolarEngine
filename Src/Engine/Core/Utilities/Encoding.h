#pragma once
#include "Core/API.h"
#include "Core/Types/Variable.h"
#include "Core/Types/Collections/List.h"

//-------------------------------------------------------------------------

namespace SE::Encoding
{
	enum class EncodingType
	{
		ANSI,
		Unicode,
		UnicodeBigEndian,
		UTF8
	};

    //-------------------------------------------------------------------------
    // Base 64 Encoding
    //-------------------------------------------------------------------------

    namespace Base64
    {
		SE_API_CORE int32 EncodeLength(int32 size);
		SE_API_CORE int32 DecodeLength(const char* encoded, int32 length);
        SE_API_CORE void Encode(const byte* bytes, int32 dataSize, List<char>& output);
		SE_API_CORE void Encode(const byte* bytes, int32 dataSize, char* encodeData);
        SE_API_CORE void Decode(const char* encoded, int32 dataSize, List<byte>& output);
		SE_API_CORE void Decode(const char* encoded, int32 dataSize, byte* output);
    }

    //-------------------------------------------------------------------------
    // Base 85 Encoding
    //-------------------------------------------------------------------------

    namespace Base85
    {
        SE_API_CORE List<uint8> Encode(uint8 const* pDataToEncode, uint64 dataSize);
        SE_API_CORE List<uint8> Decode(uint8 const* pDataToDecode, uint64 dataSize);
    }


	/// <summary>
	/// Encrypt bytes with custom data
	/// </summary>
	/// <param name="data">Bytes to encrypt</param>
	/// <param name="size">Amount of bytes to process</param>
	SE_API_CORE void EncryptBytes(byte* data, uint64 size);

	/// <summary>
	/// Decrypt bytes with custom data
	/// </summary>
	/// <param name="data">Bytes to decrypt</param>
	/// <param name="size">Amount of bytes to process</param>
	SE_API_CORE void DecryptBytes(byte* data, uint64 size);

}