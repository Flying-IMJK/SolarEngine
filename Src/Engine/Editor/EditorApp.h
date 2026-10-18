#pragma once

#include "Editor/API.h"
#include "Runtime/App.h"

namespace SE
{
	class ProjectInfo;
	class SpriteAtlas;
}

namespace SE::Editor
{
	class ManagedEditor;

	struct IconPack;

	class SE_API_EDITOR EditorApp : public App
	{
		constexpr static float s_windowControlButtonWidth = 45;
		constexpr static float s_minimumDraggableGap = 24; // Minimum open gap left open to allow dragging
		constexpr static float s_sectionPadding = 8;       // Padding between the window frame/window controls and the menu/control sections

		static float GetWindowsControlsWidth()
		{
			return s_windowControlButtonWidth * 3;
		}
	public:

		static EditorApp& Ins();

		ManagedEditor* managedEditor = nullptr;

		/// <summary>
		/// Information about the loaded game project.
		/// </summary>
		ProjectInfo* Project = nullptr;

	public:
		int32 LoadProduct() override;
		Window* CreateMainWindow()  override;
		Window* GetMainWindow() override;
		bool Init()  override;
		void BeforeRun()  override;
		void BeforeExit()  override;

		void OnUpdate() override;
		void OnLateUpdate() override;
		void OnRender() override;

	private:

		bool m_IconAtlasUpload;
		SpriteAtlas* m_IconAtlas = nullptr;
		List<IconPack*> m_LoadedIcons;

		/*void BorderlessWindowHitTest(const Float2& mouse, WindowHitCodes& hit, bool& handled);
		void DrawToolBar();
		void DrawWindowControls();*/

		void ImportAsset();
		void LoadIcon();
		void UploadIcon();
	};

	SE_CLASS(API(NoSpawn, Name="Editor"))
	class ManagedEditor : ScriptingObject
	{
		SCRIPTING_TYPE_NO_SPAWN(ManagedEditor);

		static UID ObjectID;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="ManagedEditor"/> class.
		/// </summary>
		ManagedEditor();

		/// <summary>
		/// Finalizes an instance of the <see cref="ManagedEditor"/> class.
		/// </summary>
		~ManagedEditor() override;


		void Init();
		void Update();
		void LateUpdate();
		void Render();
		void Exit();


		/// <summary>
		/// Gets the main window created by the c# editor.
		/// </summary>
		/// <returns>The main window</returns>
		Window* GetMainWindow();

		SE_FUNCTION(API())
		ProjectInfo* GetProjectInfo() const;

		SE_FUNCTION(API())
		void CreateDemoScene();

	private:

		void OnEditorAssemblyLoaded(CLRAssembly* assembly);
		void InvokeManagedMethod(const char* methodName);
	};

}
