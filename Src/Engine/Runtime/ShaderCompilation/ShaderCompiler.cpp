
#include "ShaderCompiler.h"
#include "ShadersCompilation.h"

#include "Runtime/Core/Types/DateTime.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"
#include "Runtime/Core/Platform/Win32/Win32CriticalSection.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Profiler/Profiler.h"

#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"


namespace SE
{
	namespace IncludedFiles
	{
		struct File
		{
			String Path;
			DateTime LastEditTime;
			StringAnsi Source;
		};

		CriticalSection Locker;
		Dictionary<String, File*> Files;
	}

	bool ShaderCompiler::Compile(ShaderCompilationContext* context)
	{
		// Clear cache
		m_GlobalMacros.Clear();
		m_Macros.Clear();
		m_ConstantBuffers.Clear();
		m_GlobalMacros.EnsureCapacity(32);
		m_Macros.EnsureCapacity(32);
		m_Context = context;

		// Prepare
		auto output = context->Output;
		auto meta = context->Meta;
		const int32 shadersCount = meta->GetShadersCount();
		if (!OnCompileBegin())
		{
			return false;
		}

		m_GlobalMacros.Add({ nullptr, nullptr });

		// Setup constant buffers cache
		m_ConstantBuffers.EnsureCapacity(meta->CB.Count(), false);
		for (int32 i = 0; i < meta->CB.Count(); i++)
		{
			m_ConstantBuffers.Add({ meta->CB[i].Slot, false, 0 });
		}

		// [Output] Version number
		output->WriteInt32(GPU_SHADER_CACHE_VERSION);

		// [Output] Additional data start
		const int32 additionalDataStartPos = output->GetPosition();
		output->WriteInt32(-1);

		// [Output] Amount of shaders
		output->WriteInt32(shadersCount);

		// Compile all shaders
		if (!CompileShaders())
		{
			return false;
		}

		// [Output] Constant Buffers
		{
			const int32 cbsCount = m_ConstantBuffers.Count();
			ASSERT(cbsCount == meta->CB.Count());

			// Find maximum used slot index
			byte maxCbSlot = 0;
			for (int32 i = 0; i < cbsCount; i++)
			{
				maxCbSlot = Math::Max(maxCbSlot, m_ConstantBuffers[i].Slot);
			}

			output->WriteByte(static_cast<byte>(cbsCount));
			output->WriteByte(maxCbSlot);
			// TODO: do we still need to serialize max cb slot?

			for (int32 i = 0; i < cbsCount; i++)
			{
				output->WriteByte(m_ConstantBuffers[i].Slot);
				output->WriteUint32(m_ConstantBuffers[i].Size);
			}
		}

		// Additional Data Start
		*(int32*)(output->GetHandle() + additionalDataStartPos) = output->GetPosition();

		// [Output] Includes
		output->WriteInt32(context->Includes.Count());
		for (auto& include : context->Includes)
		{
			String compactPath = ShadersCompilation::CompactShaderPath(include);
			output->WriteString(compactPath, 11);
			const auto date = FileSystem::GetFileLastEditTime(include);
			output->Write(date);
		}

		return true;
	}

	bool ShaderCompiler::GetIncludedFileSource(ShaderCompilationContext* context, const char* sourceFile, const char* includedFile, const char*& source, int32& sourceLength)
	{
		PROFILE_CPU();
		source = nullptr;
		sourceLength = 0;

		// Get actual file path
		const String includedFileName(includedFile);
		String path = ShadersCompilation::ResolveShaderPath(includedFileName);
		if (!FileSystem::FileExists(path))
		{
			LOG_ERROR("ShaderCompiler", "Unknown shader source file '{0}' included in '{1}'.{2}", includedFileName, StringAnsiView(sourceFile), String::Empty);
			return true;
		}

		Threading::ScopeLock lock(IncludedFiles::Locker);

		// Try to reuse file
		IncludedFiles::File* result = nullptr;
		if (!IncludedFiles::Files.TryGet(path, result) || FileSystem::GetFileLastEditTime(path) > result->LastEditTime)
		{
			// Remove old one
			if (result)
			{
				Delete(result);
				IncludedFiles::Files.Remove(path);
			}

			// Load file
			result = New<IncludedFiles::File>();
			result->Path = path;
			result->LastEditTime = FileSystem::GetFileLastEditTime(path);
			if (!File::ReadAllText(result->Path, result->Source))
			{
				LOG_ERROR("ShaderCompiler", "Failed to load shader source file '{0}' included in '{1}' (path: '{2}')",
					StringAnsiView(includedFile), StringAnsiView(sourceFile), path);
				Delete(result);
				return false;
			}
			IncludedFiles::Files.Add(path, result);
		}

		context->Includes.Add(path);

		// Copy to output
		source = result->Source.Get();
		sourceLength = result->Source.Length();
		return true;
	}

	void ShaderCompiler::DisposeIncludedFilesCache()
	{
		Threading::ScopeLock lock(IncludedFiles::Locker);

		IncludedFiles::Files.ClearDelete();
	}

	bool ShaderCompiler::CompileShaders()
	{
		auto meta = m_Context->Meta;
#if BUILD_DEBUG
#define PROFILE_COMPILE_SHADER(s) ZoneTransientN(___tracy_scoped_zone, s.Name.Get(), true);
#else
#define PROFILE_COMPILE_SHADER(s)
#endif

		// Generate vertex shaders cache
		for (int32 i = 0; i < meta->VS.Count(); i++)
		{
			auto& shader = meta->VS[i];
			ASSERT(shader.GetStage() == ShaderStage::Vertex && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
			PROFILE_COMPILE_SHADER(shader);
			if (!CompileShader(shader, &WriteCustomDataVS))
			{
				LOG_ERROR("ShaderCompiler", "Failed to compile \'{0}\'", shader.Name);
				return false;
			}
		}

		// Generate hull shaders cache
		for (int32 i = 0; i < meta->HS.Count(); i++)
		{
			auto& shader = meta->HS[i];
			ASSERT(shader.GetStage() == ShaderStage::Hull && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
			PROFILE_COMPILE_SHADER(shader);
			if (!CompileShader(shader, &WriteCustomDataHS))
			{
				LOG_ERROR("ShaderCompiler", "Failed to compile \'{0}\'", shader.Name);
				return false;
			}
		}

		// Generate domain shaders cache
		for (int32 i = 0; i < meta->DS.Count(); i++)
		{
			auto& shader = meta->DS[i];
			ASSERT(shader.GetStage() == ShaderStage::Domain && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
			PROFILE_COMPILE_SHADER(shader);
			if (!CompileShader(shader))
			{
				LOG_ERROR("ShaderCompiler", "Failed to compile \'{0}\'", shader.Name);
				return false;
			}
		}

		// Generate geometry shaders cache
		for (int32 i = 0; i < meta->GS.Count(); i++)
		{
			auto& shader = meta->GS[i];
			ASSERT(shader.GetStage() == ShaderStage::Geometry && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
			PROFILE_COMPILE_SHADER(shader);
			if (!CompileShader(shader))
			{
				LOG_ERROR("ShaderCompiler", "Failed to compile \'{0}\'", shader.Name);
				return false;
			}
		}

		// Generate pixel shaders cache
		for (int32 i = 0; i < meta->PS.Count(); i++)
		{
			auto& shader = meta->PS[i];
			ASSERT(shader.GetStage() == ShaderStage::Pixel && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
			PROFILE_COMPILE_SHADER(shader);
			if (!CompileShader(shader))
			{
				LOG_ERROR("ShaderCompiler", "Failed to compile \'{0}\'", shader.Name);
				return false;
			}
		}

		// Generate compute shaders cache
		for (int32 i = 0; i < meta->CS.Count(); i++)
		{
			auto& shader = meta->CS[i];
			ASSERT(shader.GetStage() == ShaderStage::Compute && (shader.Flags & ShaderFlags::Hidden) == (ShaderFlags)0);
			PROFILE_COMPILE_SHADER(shader);
			if (!CompileShader(shader))
			{
				LOG_ERROR("ShaderCompiler", "Failed to compile \'{0}\'", shader.Name);
				return false;
			}
		}

#undef PROFILE_COMPILE_SHADER
		return true;
	}

	bool ShaderCompiler::OnCompileBegin()
	{
		// Setup global macros
		static const char* Numbers[] =
			{
				// @formatter:off
				"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
				// @formatter:on
			};
		const auto profile = GetProfile();
		const auto featureLevel = GPUUtils::GetFeatureLevel(profile);
		m_GlobalMacros.Add({ "FEATURE_LEVEL", Numbers[(int32)featureLevel] });

		return true;
	}

	bool ShaderCompiler::OnCompileEnd()
	{
		return true;
	}

	bool ShaderCompiler::WriteShaderFunctionBegin(ShaderCompilationContext* context, ShaderFunctionMeta& meta)
	{
		auto output = context->Output;

		// [Output] Type
		output->WriteByte(static_cast<byte>(meta.GetStage()));

		// [Output] Permutations count
		output->WriteByte(meta.Permutations.Count());

		// [Output] Shader function name
		output->WriteStringAnsi(meta.Name, 11);

		// [Output] Shader flags
		output->WriteUint32(meta.Flags.Get());

		return true;
	}

	bool ShaderCompiler::WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* header, int32 headerSize, const void* cache, int32 cacheSize)
	{
		auto output = context->Output;

		// [Output] Write compiled shader cache
		output->WriteUint32(cacheSize + headerSize);
		output->WriteBytes(header, headerSize);
		output->WriteBytes(cache, cacheSize);

		// [Output] Shader bindings meta
		output->Write(bindings.instructionsCount);
		output->Write(bindings.usedCBsMask);
		output->Write(bindings.usedSRsMask);
		output->Write(bindings.usedUAsMask);

		return true;
	}

	bool ShaderCompiler::WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* cache, int32 cacheSize)
	{
		auto output = context->Output;

		// [Output] Write compiled shader cache
		output->WriteUint32(cacheSize);
		output->WriteBytes(cache, cacheSize);

		// [Output] Shader bindings meta
		output->Write(bindings.instructionsCount);
		output->Write(bindings.usedCBsMask);
		output->Write(bindings.usedSRsMask);
		output->Write(bindings.usedUAsMask);

		return true;
	}

	bool ShaderCompiler::WriteShaderFunctionEnd(ShaderCompilationContext* context, ShaderFunctionMeta& meta)
	{
		return true;
	}

	bool ShaderCompiler::WriteCustomDataVS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const List<ShaderMacro>& macros)
	{
		auto output = context->Output;
		auto& metaVS = *(VertexShaderMeta*)&meta;
		auto& layout = metaVS.InputLayout;

		// Get visible entries (based on `visible` flag switch)
		int32 layoutSize = 0;
		bool layoutVisible[VERTEX_SHADER_MAX_INPUT_ELEMENTS];
		for (int32 i = 0; i < layout.Count(); i++)
		{
			auto& element = layout[i];
			layoutVisible[i] = false;

			// Parse using all input macros
			StringAnsi value = element.VisibleFlag;
			for (int32 j = 0; j < macros.Count() - 1; j++)
			{
				if (macros[j].Name == value)
				{
					value = macros[j].Definition;
					break;
				}
			}

			if (value == "true" || value == "1")
			{
				// Visible
				layoutSize++;
				layoutVisible[i] = true;
			}
			else if (value == "false" || value == "0")
			{
				// Hidden
			}
			else
			{
				LOG_ERROR("ShaderCompiler", "Invalid option value \'{1}\' for  layout element \'visible\' flag on vertex shader \'{0}\'.", String(metaVS.Name), String(value));
				return false;
			}
		}

		// [Output] Input Layout
		output->WriteByte(static_cast<byte>(layoutSize));
		for (int32 a = 0; a < layout.Count(); a++)
		{
			auto& element = layout[a];
			if (!layoutVisible[a])
				continue;
			GPUShaderProgramVS::InputElement data;
			data.Type = static_cast<byte>(element.Type);
			data.Index = element.Index;
			data.Format = static_cast<byte>(element.Format);
			data.InputSlot = element.InputSlot;
			data.AlignedByteOffset = element.AlignedByteOffset;
			data.InputSlotClass = element.InputSlotClass;
			data.InstanceDataStepRate = element.InstanceDataStepRate;
			output->Write(data);
		}

		return true;
	}

	bool ShaderCompiler::WriteCustomDataHS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const List<ShaderMacro>& macros)
	{
		auto output = context->Output;
		auto& metaHS = *(HullShaderMeta*)&meta;

		// [Output] Control Points Count
		output->WriteInt32(metaHS.ControlPointsCount);

		return true;
	}

	void ShaderCompiler::GetDefineForFunction(ShaderFunctionMeta& meta, List<ShaderMacro>& macros)
	{
		auto& functionName = meta.Name;
		const int32 functionNameLength = static_cast<int32>(functionName.Length());
		m_FuncNameDefineBuffer.Clear();
		m_FuncNameDefineBuffer.EnsureCapacity(functionNameLength + 2);
		m_FuncNameDefineBuffer.Add('_');
		m_FuncNameDefineBuffer.Add(functionName.Get(), functionNameLength);
		m_FuncNameDefineBuffer.Add('\0');
		macros.Add({ m_FuncNameDefineBuffer.Get(), "1" });
	}
} // SE