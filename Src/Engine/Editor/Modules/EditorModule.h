#pragma once


namespace SE::Editor
{
	class EditorApp;

	/// <summary>
	/// Base class for all Editor modules.
	/// </summary>
	class EditorModule
	{
	public:
		virtual ~EditorModule() = default;
		/// <summary>
		/// Gets the initialization order. Lower first ordering.
		/// </summary>
		virtual int InitOrder() = 0;

		EditorApp* editor;

		/// <summary>
		/// Gets the editor undo.
		/// </summary>
//		EditorUndo Undo => Editor.Undo;

		/// <summary>
		/// Called when Editor is startup up. Performs module initialization.
		/// </summary>
		virtual void OnInit()
		{
		}

		/// <summary>
		/// Called when Editor is ready and will start work.
		/// </summary>
		virtual void OnEndInit()
		{
		}

		/// <summary>
		/// Called when every Editor update tick.
		/// </summary>
		virtual void OnUpdate()
		{
		}

		/// <summary>
		/// Called when Editor is closing. Performs module cleanup.
		/// </summary>
		virtual void OnExit()
		{
		}

		/// <summary>
		/// Called before Editor will enter play mode.
		/// </summary>
		virtual void OnPlayBeginning()
		{
		}

		/// <summary>
		/// Called when Editor is entering play mode.
		/// </summary>
		virtual void OnPlayBegin()
		{
		}

		/// <summary>
		/// Called when Editor leaves the play mode.
		/// </summary>
		virtual void OnPlayEnd()
		{
		}

	protected:
		/// <summary>
		/// Initializes a new instance of the <see cref="EditorModule"/> class.
		/// </summary>
		/// <param name="editor">The editor.</param>
		EditorModule(EditorApp* editor) : editor(editor)
		{

		}
	};

} // SE

