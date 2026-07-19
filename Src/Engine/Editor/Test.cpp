
#include "Test.h"

#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/EngineContext.h"
#include "Runtime/ShaderReflection.h"
#include "Runtime/Level/Level.h"
#include "Runtime/Level/Actors/DirectionalLight.h"
#include "Runtime/Level/Actors/PointLight.h"
#include "Runtime/Level/Actors/Sky.h"
#include "Runtime/Level/Actors/StaticModel.h"

#include "Runtime/Resource/AssetContent.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE::Editor
{

	Sky* m_Sky;
	StaticModel* m_SphereModel;
	StaticModel* m_SphereModel1;
	StaticModel* m_PlaneModel;
	DirectionalLight* m_directionalLight;
	PointLight* m_PointLight;

	void CreateTestScene()
	{
		if (Level::Scenes.Count() < 0)
		{
			return;
		}

		String cubeMapOutPutPath = EngineContext::ProjectContentFolder + SE_TEXT("/Skybox.sge");
		CubeTexture* cubeTexture = AssetContent::Load<CubeTexture>(cubeMapOutPutPath);

		Scene* scene = Level::Scenes[0];

		if (scene == nullptr)
		{
			return;
		}

		m_Sky = New<Sky>();
		m_SphereModel = New<StaticModel>();
		m_SphereModel1 = New<StaticModel>();
		m_PlaneModel = New<StaticModel>();
		m_directionalLight = New<DirectionalLight>();
		m_PointLight = New<PointLight>();

		m_Sky->SetName(SE_TEXT("Sky"));
		m_SphereModel->SetName(SE_TEXT("Sphere"));
		m_SphereModel1->SetName(SE_TEXT("Sphere1"));
		m_PlaneModel->SetName(SE_TEXT("Plane"));
		m_directionalLight->SetName(SE_TEXT("DirectionalLight"));
		m_PointLight->SetName(SE_TEXT("PointLight"));

		// directionalLight->Color = Colors::Blue;
		m_directionalLight->SetPosition({ 2, 2, 0});
		m_directionalLight->SetOrientation(Quaternion::Euler(90, 0, 45));

		m_PointLight->SetPosition({ 10, 2, 0});
		m_PointLight->SourceRadius = 20;
		m_PointLight->SourceLength = 10;

		m_PlaneModel->SetPosition({ 0, -4, 0});
		m_PlaneModel->SetScale({ 5, 5, 1});
		m_PlaneModel->SetOrientation(Quaternion::Euler(90, 0, 0));

		m_SphereModel1->SetPosition({0, 0, 3});
		m_SphereModel->SetPosition({ 0, 0, 0});

		Level::SpawnActor(m_Sky, scene);
		Level::SpawnActor(m_SphereModel, scene);
		Level::SpawnActor(m_SphereModel1, scene);
		Level::SpawnActor(m_PlaneModel, scene);
		// Level::SpawnActor(m_directionalLight, scene);
		Level::SpawnActor(m_PointLight, scene);

		Model* sphereAsset = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/Sphere"));
		Model* planeAsset = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/plane"));
		sphereAsset->WaitForLoaded();
		planeAsset->WaitForLoaded();
		m_SphereModel->Model.Set(sphereAsset);
		m_SphereModel1->Model.Set(sphereAsset);
		m_PlaneModel->Model.Set(planeAsset);
		m_Sky->CubeTexture.Set(cubeTexture);
	}

	/*class TestSystem final : public ISystem
	{
		ENGINE_SYSTEM(TestSystem)

		TestSystem() : ISystem(SE_TEXT("Test"), 1000)
		{

		}


		struct DrawVert
		{
			Float2  pos{};
			Float2 uv{};
			Color32 col;
		public:
			DrawVert(){}

			DrawVert(Float2 pos, Float2 uv, Color32 color32) : pos(pos), uv(uv), col(color32)
			{

			}

			DrawVert(DrawVert const &vert)
			{
				pos = vert.pos;
				col = vert.col;
			}
		};

		GPUPipelineState* m_PipelineState;
		Shader* m_Shader;
		DynamicIndexBuffer* m_IndexBuffer;
		DynamicVertexBuffer* m_VertexBuffer;
		GPUSampler* m_Sampler;
		List<DrawVert> m_Vert;
		List<uint16> m_Index;
		AssetRef<Texture> testImage;

		Sky* m_Sky;
		StaticModel* m_SphereModel;
		StaticModel* m_SphereModel1;
		StaticModel* m_PlaneModel;
		DirectionalLight* m_directionalLight;
		PointLight* m_PointLight;

		Slang::ComPtr<slang::IGlobalSession> globalSession;

		bool OnInit() override
		{
			/*slang::createGlobalSession(globalSession.writeRef());

			SlangTest();#1#

			/*String testImportPath = EngineContext::ProjectContentFolder + SE_TEXT("/TestPs1.png");
			String testOutPutPath = EngineContext::ProjectContentFolder + SE_TEXT("/TestPs1.sge");

			AssetsImporting::Import(testImportPath, testOutPutPath);#1#

			String cubeMapImportPath = EngineContext::ProjectContentFolder + SE_TEXT("/SkyBox.png");
			String cubeMapOutPutPath = EngineContext::ProjectContentFolder + SE_TEXT("/Skybox.sge");
			TextureUtils::Options importOptions;
			importOptions.Type = TextureFormatType::CubeMap;
			importOptions.GenerateMipMaps = false;
			importOptions.Compress = false;

			AssetsImporting::Import(cubeMapImportPath, cubeMapOutPutPath, &importOptions);

			m_Sampler = GPUDevice::instance->CreateSampler();
			GPUSamplerDescription samplerDescription = GPUSamplerDescription::New(GPUSamplerFilter::Bilinear, GPUSamplerAddressMode::Wrap);
			m_Sampler->Init(samplerDescription);


			// m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Test"));
			// m_PipelineState = GPUPipelineState::New();
			//
			// m_Shader->OnLoadedEvent.Bind([&](Asset* asset)
			// {
			// 	Shader* shader = DynamicCast<Shader, Asset>(asset);
			// 	GPUPipelineState::Description description = GPUPipelineState::Description::Default;
			// 	description.VS = shader->GetShader()->GetVS(SE_TEXT("VS"));
			// 	description.PS = shader->GetShader()->GetPS(SE_TEXT("PS"));
			//
			// 	m_PipelineState->Init(description);
			// });


			m_IndexBuffer = New<DynamicIndexBuffer>(10, sizeof(uint16), SE_TEXT("TestIndexBuffer"));
			m_VertexBuffer = New<DynamicVertexBuffer>(10, sizeof(DrawVert), SE_TEXT("TestVertexBuffer"));

			m_Vert.Add(DrawVert{Float2{0.5f, 0.5f}, {1.0, 1.0}, Color32(Color(1.0f, 0.0f, 0.0f, 1.0))});
			m_Vert.Add(DrawVert{Float2{0.5f, -0.5f}, {1.0, 0.0}, Color32(Color(0.0f, 1.0f, 0.0f, 1.0))});
			m_Vert.Add(DrawVert{Float2{-0.5f, -0.5f}, {0.0, 0.0}, Color32(Color(0.0f, 0.0f, 1.0f, 1.0))});
			m_Vert.Add(DrawVert{Float2{-0.5f, 0.5f}, {0.0, 1.0}, Color32(Color(0.0f, 0.0f, 1.0f, 1.0))});

			m_Index = {
				0, 2, 1, 2, 0, 3
			};

			// testImage = AssetContent::Load<Texture>(testOutPutPath);

			CubeTexture* cubeTexture = AssetContent::Load<CubeTexture>(cubeMapOutPutPath);

			Scene* scene = New<Scene>();
			Level::Scenes.Add(scene);

			m_Sky = New<Sky>();
			m_SphereModel = New<StaticModel>();
			m_SphereModel1 = New<StaticModel>();
			m_PlaneModel = New<StaticModel>();
			m_directionalLight = New<DirectionalLight>();
			m_PointLight = New<PointLight>();

			m_Sky->SetName(SE_TEXT("Sky"));
			m_SphereModel->SetName(SE_TEXT("Sphere"));
			m_SphereModel1->SetName(SE_TEXT("Sphere1"));
			m_PlaneModel->SetName(SE_TEXT("Plane"));
			m_directionalLight->SetName(SE_TEXT("DirectionalLight"));
			m_PointLight->SetName(SE_TEXT("PointLight"));

			// directionalLight->Color = Colors::Blue;
			m_directionalLight->SetPosition({ 2, 2, 0});
			m_directionalLight->SetOrientation(Quaternion::Euler(90, 0, 45));

			m_PointLight->SetPosition({ 10, 2, 0});
			m_PointLight->SourceRadius = 20;
			m_PointLight->SourceLength = 10;

			m_PlaneModel->SetPosition({ 0, -4, 0});
			m_PlaneModel->SetScale({ 5, 5, 1});
			m_PlaneModel->SetOrientation(Quaternion::Euler(90, 0, 0));

			m_SphereModel1->SetPosition({0, 0, 3});

			Level::SpawnActor(m_Sky, scene);
			Level::SpawnActor(m_SphereModel, scene);
			Level::SpawnActor(m_SphereModel1, scene);
			Level::SpawnActor(m_PlaneModel, scene);
			// Level::SpawnActor(m_directionalLight, scene);
			Level::SpawnActor(m_PointLight, scene);

			Level::SpawnActor(scene);

			Model* sphereAsset = AssetContent::Load<Model>(EngineContext::ProjectContentFolder + SE_TEXT("/Sphere.sge"));
			Model* planeAsset = AssetContent::Load<Model>(EngineContext::ProjectContentFolder + SE_TEXT("/Plane.sge"));
			m_SphereModel->Model.Set(sphereAsset);
			m_SphereModel1->Model.Set(sphereAsset);
			m_PlaneModel->Model.Set(planeAsset);
			m_Sky->CubeTexture.Set(cubeTexture);

			return true;
		}

		void OnRender() override
		{
			Gizmos::DrawBox({0, 5, 0}, Quaternion::Identity, {2, 2, 2}, Colors::Yellow);
			Gizmos::DrawWireBox({5, 0, 0}, Quaternion::Identity, {2, 2, 2}, Colors::Red);
		}

		void SlangTest()
		{
			SlangCompileRequest* compileRequest = CreateSlangCompileRequest();

			SlangResult slangResult = spCompile(compileRequest);
			StringAnsiView log = spGetDiagnosticOutput(compileRequest);

			Slang::ComPtr<slang::IComponentType> pSlangGlobalScope;
			spCompileRequest_getProgram(compileRequest, pSlangGlobalScope.writeRef());

			Slang::ComPtr<slang::ISession> pSlangSession(pSlangGlobalScope->getSession());

			List<Slang::ComPtr<slang::IComponentType>> pSlangEntryPoints;

			Slang::ComPtr<slang::IComponentType>& pSlangEntryPoint1 = pSlangEntryPoints.AddOne();
			spCompileRequest_getEntryPoint(compileRequest, 0, pSlangEntryPoint1.writeRef());

			Slang::ComPtr<slang::IComponentType>& pSlangEntryPoint2 = pSlangEntryPoints.AddOne();
			spCompileRequest_getEntryPoint(compileRequest, 1, pSlangEntryPoint2.writeRef());


			Slang::ComPtr<slang::IComponentType> pSlangProgram;
			spCompileRequest_getProgram(compileRequest, pSlangProgram.writeRef());



			ShaderContent shaderContent;
			shaderContent.slangGlobalScope = pSlangGlobalScope;
			shaderContent.slangEntryPoints.Add(pSlangEntryPoint1);
			shaderContent.slangEntryPoints.Add(pSlangEntryPoint2);

			List<slang::EntryPointLayout*> pSlangEntryPointReflectors;

			pSlangEntryPointReflectors.Add(pSlangEntryPoint1->getLayout()->getEntryPointByIndex(0));
			pSlangEntryPointReflectors.Add(pSlangEntryPoint2->getLayout()->getEntryPointByIndex(0));


			auto pSlangGlobalScopeLayout = pSlangGlobalScope->getLayout();

			const ShaderReflection* shaderReflection = ShaderReflection::Create(&shaderContent, pSlangGlobalScopeLayout, pSlangEntryPointReflectors);
		}

		enum class SlangCompilerFlags
		{
			None = 0x0,
			TreatWarningsAsErrors = 0x1,
			/// Enable dumping of intermediate artifacts during compilation.
			/// Note that if a shader is cached no artifacts are being produced.
			/// Delete the `.shadercache` directory in the build directory before dumping.
			DumpIntermediates = 0x2,
			FloatingPointModeFast = 0x4,
			FloatingPointModePrecise = 0x8,
			GenerateDebugInfo = 0x10,
			MatrixLayoutColumnMajor = 0x20, // Falcor is using row-major, use this only to compile stand-alone external shaders.
		};


		SlangCompileRequest* CreateSlangCompileRequest()
		{
			slang::IGlobalSession* pSlangGlobalSession = globalSession;

			slang::SessionDesc sessionDesc;

			List<const char*> slangSearchPaths;

			sessionDesc.searchPaths = slangSearchPaths.Get();
			sessionDesc.searchPathCount = (SlangInt)slangSearchPaths.Count();

			slang::TargetDesc targetDesc;
			targetDesc.format = SLANG_TARGET_UNKNOWN;
			targetDesc.profile = pSlangGlobalSession->findProfile("sm_6_0");

			EnumFlags<SlangCompilerFlags> compilerFlags;
			// Set floating point mode. If no shader compiler flags for this were set, we use Slang's default mode.
			bool flagFast = compilerFlags.IsFlag(SlangCompilerFlags::FloatingPointModeFast);
			bool flagPrecise = compilerFlags.IsFlag(SlangCompilerFlags::FloatingPointModePrecise);
			if (flagFast && flagPrecise)
			{
				flagFast = false;
			}

			SlangFloatingPointMode slangFpMode = SLANG_FLOATING_POINT_MODE_DEFAULT;
			if (flagFast)
				slangFpMode = SLANG_FLOATING_POINT_MODE_FAST;
			else if (flagPrecise)
				slangFpMode = SLANG_FLOATING_POINT_MODE_PRECISE;

			targetDesc.floatingPointMode = slangFpMode;
			targetDesc.forceGLSLScalarBufferLayout = true;

			targetDesc.floatingPointMode = slangFpMode;

			targetDesc.forceGLSLScalarBufferLayout = true;
			targetDesc.format = SLANG_SPIRV;

			targetDesc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

			sessionDesc.targets = &targetDesc;
			sessionDesc.targetCount = 1;


			// Setup additional compiler options.
			List<slang::CompilerOptionEntry> compilerOptionEntries;
			auto addIntOption = [&compilerOptionEntries](slang::CompilerOptionName name, int value) {
			    compilerOptionEntries.Add({name, {slang::CompilerOptionValueKind::Int, 1, value, nullptr, nullptr}});
			};
			auto addStringOption = [&compilerOptionEntries](slang::CompilerOptionName name, const char* value) {
			    compilerOptionEntries.Add({name, {slang::CompilerOptionValueKind::String, 0, 0, value, nullptr}});
			};

			// We always use row-major matrix layout in Falcor so by default that's what we pass to Slang
			// to allow it to compute correct reflection information. Slang then invokes the downstream compiler.
			// Column major option can be useful when compiling external shader sources that don't depend
			// on anything Falcor.
			bool useColumnMajor = compilerFlags.IsFlag(SlangCompilerFlags::MatrixLayoutColumnMajor);
			addIntOption(useColumnMajor ? slang::CompilerOptionName::MatrixLayoutColumn : slang::CompilerOptionName::MatrixLayoutRow, 1);

			// New versions of slang default to short-circuiting for logical and/or operators.
			// Facor is still written with the assumption that these operators do not short-circuit.
			// We want to transition to the new behavior, but for now we disable it.
			addIntOption(slang::CompilerOptionName::DisableShortCircuit, 1);

			// Disable noisy warnings enabled in newer slang versions.
			addStringOption(slang::CompilerOptionName::DisableWarning, "15602"); // #pragma once in modules
			addStringOption(slang::CompilerOptionName::DisableWarning, "30056"); // non-short-circuiting `?:` operator is deprecated, use 'select'
			                                                                     // instead
			addStringOption(slang::CompilerOptionName::DisableWarning, "30081"); // implicit conversion
			addStringOption(slang::CompilerOptionName::DisableWarning, "41203"); // reinterpret<> into not equally sized types

			sessionDesc.compilerOptionEntries = compilerOptionEntries.Get();
			sessionDesc.compilerOptionEntryCount = compilerOptionEntries.Count();

			Slang::ComPtr<slang::ISession> pSlangSession;
			pSlangGlobalSession->createSession(sessionDesc, pSlangSession.writeRef());

			SlangCompileRequest* pSlangRequest = nullptr;
			pSlangSession->createCompileRequest(&pSlangRequest);


			// Set debug level
			// if (mGenerateDebugInfo || is_set(program.mDesc.compilerFlags, SlangCompilerFlags::GenerateDebugInfo))
				spSetDebugInfoLevel(pSlangRequest, SLANG_DEBUG_INFO_LEVEL_STANDARD);

			// Configure any flags for the Slang compilation step
			SlangCompileFlags slangFlags = 0;

			// When we invoke the Slang compiler front-end, skip code generation step
			// so that the compiler does not complain about missing arguments for
			// specialization parameters.
			//
			slangFlags |= SLANG_COMPILE_FLAG_NO_CODEGEN;

			spSetCompileFlags(pSlangRequest, slangFlags);

			String path = EngineContext::ProjectFolder + SE_TEXT("/Shaders/Test.shader");
			StringAnsi code;
			if (!File::ReadAllText(path, code))
			{
				spDestroyCompileRequest(pSlangRequest);
				return nullptr;
			}

			int translationUnitIndex = spAddTranslationUnit(pSlangRequest, SLANG_SOURCE_LANGUAGE_SLANG, "Test");
			StringAnsi slangPath = path.ToStringAnsi();
			spAddTranslationUnitSourceString(pSlangRequest, translationUnitIndex, slangPath.Get(), code.Get());

			spAddEntryPoint(pSlangRequest, 0, "VS1", SLANG_STAGE_VERTEX);
			spAddEntryPoint(pSlangRequest, 0, "PS1", SLANG_STAGE_FRAGMENT);

			return pSlangRequest;
		}
	};

	ENGINE_SYSTEM_REGISTER(TestSystem);*/

}

