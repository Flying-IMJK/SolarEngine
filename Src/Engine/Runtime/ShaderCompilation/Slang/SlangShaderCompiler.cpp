#include "SlangShaderCompiler.h"

#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Writer.h"
#include "Runtime/ShaderCompilation/Slang/SlangReflectionBuilder.h"
#include "Runtime/ShaderCompilation/Slang/SlangShaderFileSystem.h"

#include <slang-com-ptr.h>
#include <slang-tag-version.h>
#include <slang.h>

namespace SE
{
	namespace
	{
		static const char* SolarSlangPreamble = R"(
[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderProgramAttribute {};

[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderStageAttribute
{
	string stage;
	string entry;
};

[__AttributeUsage(_AttributeTargets.Struct)]
struct SolarShaderMacroGroupAttribute
{
	string members;
};

#define SHADER_VS(entry) [SolarShaderStage("vertex", #entry)]
#define SHADER_HS(entry) [SolarShaderStage("hull", #entry)]
#define SHADER_DS(entry) [SolarShaderStage("domain", #entry)]
#define SHADER_GS(entry) [SolarShaderStage("geometry", #entry)]
#define SHADER_PS(entry) [SolarShaderStage("fragment", #entry)]
#define SHADER_CS(entry) [SolarShaderStage("compute", #entry)]
#define SHADER_MACRO(...) [SolarShaderMacroGroup(#__VA_ARGS__)]
#define SHADER_PROGRAM(name, context) [SolarShaderProgram] context struct name {}
)";

		String FromUtf8(const char* text)
		{
			return text ? StringAnsi(text).ToString() : String::Empty;
		}

		String FromUtf8(const char* text, const size_t length)
		{
			return text ? StringAnsi(text, (int32)length).ToString() : String::Empty;
		}

		bool SlangSucceeded(const SlangResult result)
		{
			return SLANG_SUCCEEDED(result);
		}

		bool IsAttribute(slang::Attribute* attribute, const char* name)
		{
			if (attribute == nullptr)
			{
				return false;
			}
			const char* attributeName = attribute->getName();
			if (attributeName == nullptr)
			{
				return false;
			}
			const String actual = FromUtf8(attributeName);
			const String expected = FromUtf8(name);
			return actual == expected || actual == expected + SE_TEXT("Attribute");
		}

		bool HasAttribute(slang::TypeReflection* type, const char* name)
		{
			if (type == nullptr)
			{
				return false;
			}
			for (unsigned int i = 0; i < type->getUserAttributeCount(); i++)
			{
				if (IsAttribute(type->getUserAttributeByIndex(i), name))
				{
					return true;
				}
			}
			return false;
		}

		SlangStage ToSlangStage(const SlangShaderStage stage)
		{
			switch (stage)
			{
			case SlangShaderStage::Vertex:
				return SLANG_STAGE_VERTEX;
			case SlangShaderStage::Hull:
				return SLANG_STAGE_HULL;
			case SlangShaderStage::Domain:
				return SLANG_STAGE_DOMAIN;
			case SlangShaderStage::Geometry:
				return SLANG_STAGE_GEOMETRY;
			case SlangShaderStage::Pixel:
				return SLANG_STAGE_FRAGMENT;
			case SlangShaderStage::Compute:
				return SLANG_STAGE_COMPUTE;
			default:
				return SLANG_STAGE_NONE;
			}
		}

		const char* GetProfileName(const ShaderCompileShaderModel shaderModel)
		{
			switch (shaderModel)
			{
			case ShaderCompileShaderModel::SM_6_6:
				return "sm_6_6";
			case ShaderCompileShaderModel::SM_6_0:
				return "sm_6_0";
			case ShaderCompileShaderModel::SM_5_0:
				return "sm_5_0";
			default:
				return "sm_5_0";
			}
		}

		String MakeCompilerBuildTag()
		{
			return String(SE_TEXT("SolarSlangCompiler/1;Slang/")) + StringAnsi(SLANG_TAG_VERSION).ToString() + SE_TEXT(";SLC2/2");
		}

		void SplitMacroGroup(const String& raw, ShaderVariantGroup& group)
		{
			group.Members.Clear();
			int32 begin = 0;
			while (begin <= raw.Length())
			{
				int32 end = begin;
				while (end < raw.Length() && raw.Get()[end] != SE_TEXT(','))
				{
					end++;
				}
				int32 left = begin;
				int32 right = end;
				while (left < right && (raw.Get()[left] == SE_TEXT(' ') || raw.Get()[left] == SE_TEXT('\t') || raw.Get()[left] == SE_TEXT('\r') || raw.Get()[left] == SE_TEXT('\n')))
				{
					left++;
				}
				while (right > left && (raw.Get()[right - 1] == SE_TEXT(' ') || raw.Get()[right - 1] == SE_TEXT('\t') || raw.Get()[right - 1] == SE_TEXT('\r') || raw.Get()[right - 1] == SE_TEXT('\n')))
				{
					right--;
				}
				if (right > left)
				{
					group.Members.Add(raw.Substring(left, right - left));
				}
				if (end == raw.Length())
				{
					break;
				}
				begin = end + 1;
			}
			group.DefaultMember = group.Members.Count() > 0 ? group.Members[0] : String::Empty;
		}

		bool HasStage(const SlangProgramDeclaration& program, const SlangShaderStage stage)
		{
			for (int32 i = 0; i < program.Stages.Count(); i++)
			{
				if (program.Stages[i].Stage == stage)
				{
					return true;
				}
			}
			return false;
		}

		bool ValidateStageContract(const SlangProgramDeclaration& program, String& error)
		{
			for (int32 i = 0; i < program.Stages.Count(); i++)
			{
				for (int32 j = i + 1; j < program.Stages.Count(); j++)
				{
					if (program.Stages[i].Stage == program.Stages[j].Stage)
					{
						error = SE_TEXT("Shader program has duplicated stage: ") + ToString(program.Stages[i].Stage);
						return false;
					}
				}
			}

			const bool hasCS = HasStage(program, SlangShaderStage::Compute);
			if (hasCS)
			{
				if (program.Stages.Count() != 1)
				{
					error = SE_TEXT("Compute shader program can only contain CS.");
					return false;
				}
				return true;
			}

			if (!HasStage(program, SlangShaderStage::Vertex))
			{
				error = SE_TEXT("Graphics shader program must contain VS.");
				return false;
			}
			if (!HasStage(program, SlangShaderStage::Pixel))
			{
				error = SE_TEXT("Graphics shader program must contain PS.");
				return false;
			}
			const bool hasHS = HasStage(program, SlangShaderStage::Hull);
			const bool hasDS = HasStage(program, SlangShaderStage::Domain);
			if (hasHS != hasDS)
			{
				error = SE_TEXT("HS and DS must appear together.");
				return false;
			}
			return true;
		}

		bool DiscoverPrograms(slang::IModule* module, List<SlangProgramDeclaration>& programs, String& error)
		{
			programs.Clear();
			if (module == nullptr)
			{
				error = SE_TEXT("Slang module is null.");
				return false;
			}

			slang::DeclReflection* moduleDecl = module->getModuleReflection();
			if (moduleDecl == nullptr)
			{
				error = SE_TEXT("Slang module reflection is null.");
				return false;
			}

			for (unsigned int childIndex = 0; childIndex < moduleDecl->getChildrenCount(); childIndex++)
			{
				slang::DeclReflection* child = moduleDecl->getChild(childIndex);
				if (child == nullptr || child->getKind() != slang::DeclReflection::Kind::Struct)
				{
					continue;
				}

				slang::TypeReflection* type = child->getType();
				if (!HasAttribute(type, "SolarShaderProgram"))
				{
					continue;
				}

				const char* programName = child->getName();
				if (programName == nullptr || programName[0] == '\0')
				{
					error = SE_TEXT("SHADER_PROGRAM declaration has empty name.");
					return false;
				}

				SlangProgramDeclaration program;
				program.ProgramId = FromUtf8(programName);

				for (int32 existingIndex = 0; existingIndex < programs.Count(); existingIndex++)
				{
					if (programs[existingIndex].ProgramId == program.ProgramId)
					{
						error = SE_TEXT("Duplicated SHADER_PROGRAM name: ") + program.ProgramId;
						return false;
					}
				}

				for (unsigned int attrIndex = 0; attrIndex < type->getUserAttributeCount(); attrIndex++)
				{
					slang::Attribute* attribute = type->getUserAttributeByIndex(attrIndex);
					if (IsAttribute(attribute, "SolarShaderStage"))
					{
						if (attribute->getArgumentCount() != 2)
						{
							error = SE_TEXT("SolarShaderStage attribute must have stage and entry arguments.");
							return false;
						}
						size_t stageSize = 0;
						size_t entrySize = 0;
						const char* stageText = attribute->getArgumentValueString(0, &stageSize);
						const char* entryText = attribute->getArgumentValueString(1, &entrySize);
						if (stageText == nullptr || entryText == nullptr)
						{
							error = SE_TEXT("SolarShaderStage attribute has invalid string argument.");
							return false;
						}

						SlangProgramStageDeclaration stageDecl;
						const String stageName = FromUtf8(stageText, stageSize);
						const StringAnsi stageNameAnsi(stageName);
						if (!ParseSlangShaderStage(stageNameAnsi.Get(), stageDecl.Stage))
						{
							error = SE_TEXT("Unknown shader stage declared on program: ") + program.ProgramId;
							return false;
						}
						stageDecl.EntryPoint = FromUtf8(entryText, entrySize);
						if (stageDecl.EntryPoint.IsEmpty())
						{
							error = SE_TEXT("Shader stage entry point is empty on program: ") + program.ProgramId;
							return false;
						}
						program.Stages.Add(stageDecl);
					}
					else if (IsAttribute(attribute, "SolarShaderMacroGroup"))
					{
						if (attribute->getArgumentCount() != 1)
						{
							error = SE_TEXT("SolarShaderMacroGroup attribute must have one string argument.");
							return false;
						}
						size_t macroSize = 0;
						const char* macroText = attribute->getArgumentValueString(0, &macroSize);
						if (macroText == nullptr)
						{
							error = SE_TEXT("SolarShaderMacroGroup attribute has invalid string argument.");
							return false;
						}
						ShaderVariantGroup group;
						SplitMacroGroup(FromUtf8(macroText, macroSize), group);
						if (group.Members.Count() == 0)
						{
							error = SE_TEXT("SHADER_MACRO group is empty on program: ") + program.ProgramId;
							return false;
						}
						program.VariantGroups.Add(group);
					}
				}

				if (!ValidateStageContract(program, error))
				{
					error = program.ProgramId + SE_TEXT(": ") + error;
					return false;
				}

				programs.Add(program);
			}

			if (programs.Count() == 0)
			{
				error = SE_TEXT("No SHADER_PROGRAM declarations found in root module scope.");
				return false;
			}

			return true;
		}

		bool SameProgramShape(const SlangProgramDeclaration& a, const SlangProgramDeclaration& b)
		{
			if (a.ProgramId != b.ProgramId || a.Stages.Count() != b.Stages.Count() || a.VariantGroups.Count() != b.VariantGroups.Count())
			{
				return false;
			}
			for (int32 i = 0; i < a.Stages.Count(); i++)
			{
				if (a.Stages[i].Stage != b.Stages[i].Stage || a.Stages[i].EntryPoint != b.Stages[i].EntryPoint)
				{
					return false;
				}
			}
			for (int32 groupIndex = 0; groupIndex < a.VariantGroups.Count(); groupIndex++)
			{
				if (a.VariantGroups[groupIndex].Members.Count() != b.VariantGroups[groupIndex].Members.Count())
				{
					return false;
				}
				for (int32 memberIndex = 0; memberIndex < a.VariantGroups[groupIndex].Members.Count(); memberIndex++)
				{
					if (a.VariantGroups[groupIndex].Members[memberIndex] != b.VariantGroups[groupIndex].Members[memberIndex])
					{
						return false;
					}
				}
			}
			return true;
		}

		SlangProgramDeclaration* FindProgram(List<SlangProgramDeclaration>& programs, const String& programId)
		{
			for (int32 i = 0; i < programs.Count(); i++)
			{
				if (programs[i].ProgramId == programId)
				{
					return &programs[i];
				}
			}
			return nullptr;
		}

		const ShaderProgramVariantSelection* FindSelection(const ShaderCompileRequest& request, const String& programId)
		{
			for (int32 i = 0; i < request.VariantSelections.Count(); i++)
			{
				if (request.VariantSelections[i].ProgramId == programId)
				{
					return &request.VariantSelections[i];
				}
			}
			return nullptr;
		}

		bool PlanProgramVariants(const ShaderCompileRequest& request, const SlangProgramDeclaration& program, List<ShaderVariantPlan>& variants, String& error)
		{
			variants.Clear();
			const ShaderProgramVariantSelection* selection = FindSelection(request, program.ProgramId);
			if (request.VariantSelections.Count() == 0)
			{
				return ShaderVariantPlanner::BuildAll(program.VariantGroups, variants, error);
			}

			if (selection == nullptr)
			{
				return true;
			}

			if (selection->Variants.Count() == 0)
			{
				error = SE_TEXT("Explicit variant selection is empty for program: ") + program.ProgramId;
				return false;
			}

			for (int32 variantIndex = 0; variantIndex < selection->Variants.Count(); variantIndex++)
			{
				ShaderVariantPlan plan;
				if (!ShaderVariantPlanner::Normalize(program.VariantGroups, selection->Variants[variantIndex].Defines, plan, error))
				{
					return false;
				}
				bool duplicated = false;
				for (int32 i = 0; i < variants.Count(); i++)
				{
					if (variants[i].Variant == plan.Variant)
					{
						duplicated = true;
						break;
					}
				}
				if (!duplicated)
				{
					variants.Add(plan);
				}
			}
			return true;
		}

		bool CreateSession(slang::IGlobalSession* globalSession, SlangShaderFileSystem& fileSystem, const ShaderCompileTarget& target, const ShaderVariantPlan& variant, Slang::ComPtr<slang::ISession>& session, String& error)
		{
			if (target.Platform == ShaderTargetPlatform::Unknown || target.Backend == ShaderGraphicsBackend::Unknown || target.ShaderModel == ShaderCompileShaderModel::Unknown)
			{
				error = SE_TEXT("Shader compile target must specify platform, backend and shader model.");
				return false;
			}
			if (target.Backend != ShaderGraphicsBackend::Vulkan)
			{
				error = SE_TEXT("Only Vulkan backend is supported by the current offline Slang compiler.");
				return false;
			}

			List<StringAnsi> macroNames;
			List<slang::PreprocessorMacroDesc> macroDescs;
			macroNames.SetCapacity(variant.Defines.Count(), false);
			macroDescs.SetCapacity(variant.Defines.Count(), false);
			for (int32 i = 0; i < variant.Defines.Count(); i++)
			{
				macroNames.Add(StringAnsi(variant.Defines[i]));
			}
			for (int32 i = 0; i < macroNames.Count(); i++)
			{
				slang::PreprocessorMacroDesc desc = {};
				desc.name = macroNames[i].Get();
				desc.value = "1";
				macroDescs.Add(desc);
			}

			slang::TargetDesc targetDesc = {};
			targetDesc.format = SLANG_SPIRV;
			targetDesc.profile = globalSession->findProfile(GetProfileName(target.ShaderModel));

			slang::SessionDesc sessionDesc = {};
			sessionDesc.targetCount = 1;
			sessionDesc.targets = &targetDesc;
			sessionDesc.preprocessorMacroCount = (SlangInt)macroDescs.Count();
			sessionDesc.preprocessorMacros = macroDescs.Count() == 0 ? nullptr : macroDescs.Get();
			sessionDesc.fileSystem = fileSystem.GetSlangFileSystem();
			sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_ROW_MAJOR;

			if (!SlangSucceeded(globalSession->createSession(sessionDesc, session.writeRef())))
			{
				error = SE_TEXT("Failed to create Slang session.");
				return false;
			}
			return true;
		}

		slang::IModule* LoadModule(slang::ISession* session, const ShaderCompileRequest& request, Slang::ComPtr<slang::IBlob>& diagnostics)
		{
			String sourcePath = request.SourcePath;
			if (sourcePath.IsEmpty())
			{
				sourcePath = SE_TEXT("memory.slang");
			}
			const String source = String(StringAnsi(SolarSlangPreamble)) + SE_TEXT("\n") + request.SourceCode;
			const StringAnsi sourcePathAnsi(sourcePath);
			const StringAnsi sourceAnsi(source);
			return session->loadModuleFromSourceString("SolarShaderModule", sourcePathAnsi.Get(), sourceAnsi.Get(), diagnostics.writeRef());
		}

		bool ValidateReflectionConstraints(slang::ProgramLayout* layout, String& error)
		{
			if (layout == nullptr)
			{
				error = SE_TEXT("Slang ProgramLayout is null.");
				return false;
			}

			for (unsigned int parameterIndex = 0; parameterIndex < layout->getParameterCount(); parameterIndex++)
			{
				slang::VariableLayoutReflection* parameter = layout->getParameterByIndex(parameterIndex);
				if (parameter == nullptr)
				{
					continue;
				}

				for (unsigned int categoryIndex = 0; categoryIndex < parameter->getCategoryCount(); categoryIndex++)
				{
					if (parameter->getCategoryByIndex(categoryIndex) == slang::ParameterCategory::PushConstantBuffer)
					{
						error = SE_TEXT("Push Constant is not supported by SolarEngine Slang offline compiler.");
						return false;
					}
				}
			}
			return true;
		}

		void SortPrograms(List<SLC2ProgramRecord>& programs)
		{
			for (int32 i = 0; i < programs.Count(); i++)
			{
				for (int32 j = i + 1; j < programs.Count(); j++)
				{
					if (programs[j].ProgramId < programs[i].ProgramId)
					{
						SLC2ProgramRecord tmp = programs[i];
						programs[i] = programs[j];
						programs[j] = tmp;
					}
				}
			}
		}
	}

	void SlangShaderCompiler::AddDiagnostic(const String& text)
	{
		if (text.IsEmpty())
		{
			return;
		}
		if (!m_Diagnostics.IsEmpty())
		{
			m_Diagnostics += SE_TEXT("\n");
		}
		m_Diagnostics += text;
	}

	void SlangShaderCompiler::AddSlangDiagnostics(void* diagnosticsBlob)
	{
		slang::IBlob* blob = static_cast<slang::IBlob*>(diagnosticsBlob);
		if (blob == nullptr || blob->getBufferPointer() == nullptr || blob->getBufferSize() == 0)
		{
			return;
		}
		AddDiagnostic(FromUtf8((const char*)blob->getBufferPointer(), blob->getBufferSize()));
	}

	ShaderCompileResult SlangShaderCompiler::Compile(const ShaderCompileRequest& request)
	{
		m_Diagnostics = String::Empty;

		ShaderCompileResult result;
		result.Status = ShaderCompileStatus::Failed;

		if (request.SourceCode.IsEmpty())
		{
			AddDiagnostic(SE_TEXT("Missing Slang shader source."));
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}
		if (request.Targets.Count() == 0)
		{
			AddDiagnostic(SE_TEXT("Missing shader compile target."));
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}

		Slang::ComPtr<slang::IGlobalSession> globalSession;
		if (!SlangSucceeded(slang::createGlobalSession(globalSession.writeRef())))
		{
			AddDiagnostic(SE_TEXT("Failed to create Slang global session."));
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}

		Slang::ComPtr<SlangShaderFileSystem> fileSystem(Slang::INIT_ATTACH, new SlangShaderFileSystem(request.SourcePath));

		ShaderVariantPlan baselineVariant;
		Slang::ComPtr<slang::ISession> baselineSession;
		String error;
		// 第一次加载 Module 时不带任何 Variant 宏，只用于发现根 Module 中稳定声明的 Program/Stage/Macro。
		if (!CreateSession(globalSession, *fileSystem, request.Targets[0], baselineVariant, baselineSession, error))
		{
			AddDiagnostic(error);
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}

		Slang::ComPtr<slang::IBlob> baselineDiagnostics;
		slang::IModule* baselineModule = LoadModule(baselineSession, request, baselineDiagnostics);
		AddSlangDiagnostics(baselineDiagnostics);
		if (baselineModule == nullptr)
		{
			AddDiagnostic(SE_TEXT("Failed to load Slang module for baseline discovery."));
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}

		List<SlangProgramDeclaration> baselinePrograms;
		if (!DiscoverPrograms(baselineModule, baselinePrograms, error))
		{
			AddDiagnostic(error);
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}

		for (int32 selectionIndex = 0; selectionIndex < request.VariantSelections.Count(); selectionIndex++)
		{
			if (FindProgram(baselinePrograms, request.VariantSelections[selectionIndex].ProgramId) == nullptr)
			{
				AddDiagnostic(SE_TEXT("Explicit variant selection references unknown program: ") + request.VariantSelections[selectionIndex].ProgramId);
				result.CompileMessage.Text = m_Diagnostics;
				return result;
			}
		}

		SLC2Artifact artifact;
		artifact.CompilerBuildTag = MakeCompilerBuildTag();

		for (int32 baselineProgramIndex = 0; baselineProgramIndex < baselinePrograms.Count(); baselineProgramIndex++)
		{
			const SlangProgramDeclaration& baselineProgram = baselinePrograms[baselineProgramIndex];
			List<ShaderVariantPlan> variants;
			if (!PlanProgramVariants(request, baselineProgram, variants, error))
			{
				AddDiagnostic(error);
				result.CompileMessage.Text = m_Diagnostics;
				return result;
			}
			if (variants.Count() == 0)
			{
				continue;
			}

			SLC2ProgramRecord programRecord;
			programRecord.ProgramId = baselineProgram.ProgramId;
			programRecord.VariantGroups = baselineProgram.VariantGroups;

			for (int32 targetIndex = 0; targetIndex < request.Targets.Count(); targetIndex++)
			{
				const ShaderCompileTarget& target = request.Targets[targetIndex];
				SLC2TargetRecord targetRecord;
				targetRecord.Target = target;
				targetRecord.TargetKey = BuildTargetKey(target);

				for (int32 variantIndex = 0; variantIndex < variants.Count(); variantIndex++)
				{
					const ShaderVariantPlan& variant = variants[variantIndex];
					Slang::ComPtr<slang::ISession> session;
					// 每个 Target/Variant 使用独立 Session，使宏定义、目标格式和 ShaderModel 互不串扰。
					if (!CreateSession(globalSession, *fileSystem, target, variant, session, error))
					{
						AddDiagnostic(error);
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					Slang::ComPtr<slang::IBlob> moduleDiagnostics;
					slang::IModule* module = LoadModule(session, request, moduleDiagnostics);
					AddSlangDiagnostics(moduleDiagnostics);
					if (module == nullptr)
					{
						AddDiagnostic(SE_TEXT("Failed to load Slang module for variant: ") + variant.Variant);
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					List<SlangProgramDeclaration> variantPrograms;
					// Variant 环境会重新加载 Module，但不允许宏改变 Program 声明形状，只允许改变 shader 实现代码。
					if (!DiscoverPrograms(module, variantPrograms, error))
					{
						AddDiagnostic(error);
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					SlangProgramDeclaration* variantProgram = FindProgram(variantPrograms, baselineProgram.ProgramId);
					if (variantProgram == nullptr || !SameProgramShape(baselineProgram, *variantProgram))
					{
						AddDiagnostic(SE_TEXT("Variant-controlled Program/Stage/Macro declarations are not allowed: ") + baselineProgram.ProgramId);
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					List<slang::IComponentType*> components;
					components.Add(module);
					List<Slang::ComPtr<slang::IEntryPoint>> entryPoints;
					entryPoints.Resize(baselineProgram.Stages.Count());
					for (int32 stageIndex = 0; stageIndex < baselineProgram.Stages.Count(); stageIndex++)
					{
						const SlangProgramStageDeclaration& stage = baselineProgram.Stages[stageIndex];
						Slang::ComPtr<slang::IBlob> entryDiagnostics;
						const StringAnsi entryName(stage.EntryPoint);
						if (!SlangSucceeded(module->findAndCheckEntryPoint(entryName.Get(), ToSlangStage(stage.Stage), entryPoints[stageIndex].writeRef(), entryDiagnostics.writeRef())))
						{
							AddSlangDiagnostics(entryDiagnostics);
							AddDiagnostic(SE_TEXT("Failed to find or check entry point: ") + stage.EntryPoint);
							result.CompileMessage.Text = m_Diagnostics;
							return result;
						}
						components.Add(entryPoints[stageIndex]);
					}

					Slang::ComPtr<slang::IComponentType> composite;
					Slang::ComPtr<slang::IBlob> compositeDiagnostics;
					if (!SlangSucceeded(session->createCompositeComponentType(components.Get(), (SlangInt)components.Count(), composite.writeRef(), compositeDiagnostics.writeRef())))
					{
						AddSlangDiagnostics(compositeDiagnostics);
						AddDiagnostic(SE_TEXT("Failed to create Slang composite component."));
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					Slang::ComPtr<slang::IComponentType> linked;
					Slang::ComPtr<slang::IBlob> linkDiagnostics;
					if (!SlangSucceeded(composite->link(linked.writeRef(), linkDiagnostics.writeRef())))
					{
						AddSlangDiagnostics(linkDiagnostics);
						AddDiagnostic(SE_TEXT("Failed to link Slang component."));
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					Slang::ComPtr<slang::IBlob> layoutDiagnostics;
					slang::ProgramLayout* layout = linked->getLayout(0, layoutDiagnostics.writeRef());
					AddSlangDiagnostics(layoutDiagnostics);
					if (layout == nullptr)
					{
						AddDiagnostic(SE_TEXT("Failed to get Slang program layout."));
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}
					if (!ValidateReflectionConstraints(layout, error))
					{
						AddDiagnostic(error);
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					SLC2VariantRecord variantRecord;
					variantRecord.Variant = variant.Variant;
					SlangReflectionBuilder reflectionBuilder;
					// linked component 已经完成当前 Target/Variant 的组合，编译期在这里固化运行时所需的反射 IR。
					if (!reflectionBuilder.Build(baselineProgram.ProgramId, targetRecord.TargetKey, variant.Variant, layout, variantRecord.Layout, error))
					{
						AddDiagnostic(error);
						result.CompileMessage.Text = m_Diagnostics;
						return result;
					}

					for (int32 stageIndex = 0; stageIndex < baselineProgram.Stages.Count(); stageIndex++)
					{
						Slang::ComPtr<slang::IBlob> code;
						Slang::ComPtr<slang::IBlob> codeDiagnostics;
						if (!SlangSucceeded(linked->getEntryPointCode(stageIndex, 0, code.writeRef(), codeDiagnostics.writeRef())))
						{
							AddSlangDiagnostics(codeDiagnostics);
							AddDiagnostic(SE_TEXT("Failed to emit target code for entry point: ") + baselineProgram.Stages[stageIndex].EntryPoint);
							result.CompileMessage.Text = m_Diagnostics;
							return result;
						}
						if (code == nullptr || code->getBufferPointer() == nullptr || code->getBufferSize() == 0)
						{
							AddDiagnostic(SE_TEXT("Slang returned empty target code for entry point: ") + baselineProgram.Stages[stageIndex].EntryPoint);
							result.CompileMessage.Text = m_Diagnostics;
							return result;
						}

						SLC2StageRecord stageRecord;
						stageRecord.Stage = baselineProgram.Stages[stageIndex].Stage;
						stageRecord.EntryPoint = baselineProgram.Stages[stageIndex].EntryPoint;
						stageRecord.Code.Set((const byte*)code->getBufferPointer(), (int32)code->getBufferSize());
						variantRecord.Stages.Add(stageRecord);
					}

					targetRecord.Variants.Add(variantRecord);
				}

				programRecord.Targets.Add(targetRecord);
			}

			artifact.Programs.Add(programRecord);
		}

		SortPrograms(artifact.Programs);

		if (!SLC2Writer::WriteDeterministic(artifact, result.SLC2Data, error))
		{
			AddDiagnostic(error);
			result.SLC2Data.Clear();
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}

		if (!SLC2Reader::ReadAndValidate(result.SLC2Data, error))
		{
			AddDiagnostic(error);
			result.SLC2Data.Clear();
			result.CompileMessage.Text = m_Diagnostics;
			return result;
		}

		result.Status = ShaderCompileStatus::Success;
		result.CompileMessage.Text = m_Diagnostics;
		return result;
	}
}
