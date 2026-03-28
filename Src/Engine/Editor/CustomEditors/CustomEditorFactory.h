#pragma once
#include "Core/TypeSystem/TypeID.h"
#include "Core/TypeSystem/Types.h"

namespace SE::Editor
{
	class CustomEditor;
	class ValueContainer;
	class CustomEditorRegister;

	/// <summary>
	/// Factory for creating custom editors based on type information.
	/// </summary>
	class CustomEditorFactory
	{
	public:
		/// <summary>
		/// Creates an editor for the specified type.
		/// </summary>
		/// <param name="type">The type to create an editor for.</param>
		/// <param name="canUseGeneric">If true, returns GenericEditor when no custom editor is found.</param>
		/// <returns>The created editor instance, or nullptr if not found and canUseGeneric is false.</returns>
		static CustomEditor* CreateEditor(TypeID type, bool canUseGeneric = true);

		/// <summary>
		/// Creates an editor for the values container.
		/// </summary>
		/// <param name="values">The values container.</param>
		/// <param name="overrideEditor">Optional override editor to use instead of automatic selection.</param>
		/// <returns>The created editor instance.</returns>
		static CustomEditor* CreateEditor(ValueContainer* values, CustomEditor* overrideEditor = nullptr);



		/// <summary>
		/// Unregisters a custom editor for a specific type.
		/// </summary>
		/// <param name="type">The type to unregister.</param>
		static void UnregisterEditor(TypeID type);

		/// <summary>
		/// Checks if a custom editor is registered for the specified type.
		/// </summary>
		/// <param name="type">The type to check.</param>
		/// <returns>True if a custom editor is registered, false otherwise.</returns>
		static bool HasCustomEditor(TypeID type);

		/// <summary>
		/// Gets the editor type for the specified value type.
		/// </summary>
		/// <param name="type">The value type.</param>
		/// <returns>The editor type ID, or TypeID::Invalid if not found.</returns>
		static TypeID GetEditorType(TypeID type);


		/// <summary>
		/// Registers a custom editor factory for a specific type.
		/// </summary>
		/// <param name="editorRegister">The type to register the editor for.</param>
		static void RegisterEditor(CustomEditorRegister* editorRegister);


	private:
		static Dictionary<StableID, CustomEditorRegister*> s_editorFactories;
		static Dictionary<TypeID, TypeID> s_editorTypeCache;
	};

	//---自定义类型Editor注册器---------------------------------------------------------
	class CustomEditorRegister
	{
	public:
		explicit CustomEditorRegister(CustomEditorRegister* pRegister);

		virtual CustomEditor* Create() = 0;
		virtual StableID ValueTypeStableID() = 0;
		virtual ~CustomEditorRegister() = default;

		StringView GetName() const;

	protected:
		const Char* (*getNameFunc)();
	};

	template <typename TEditor, typename TValueType, const Char* (*GetNameFunc)()>
	class TCustomEditorRegister final : CustomEditorRegister
	{
		static_assert(TIsBaseOf<CustomEditor, TEditor>::Value, "T is not derived from CustomEditor");
	public:
		TCustomEditorRegister() : CustomEditorRegister(this)
		{
			getNameFunc = GetNameFunc;
		}

		CustomEditor* Create() override
		{
			return New<TEditor>();
		}

		StableID ValueTypeStableID() override
		{
			return StableID::Generate<TValueType>();
		}
	};
} // namespace SE::Editor

// CustomEditorRegister
#define SE_CUSTOM_EDITOR(TypeName, ValueTypeName, __LINE__) 											\
const ::SE::Char* Get##__LINE__##Name() { return SE_TEXT(#TypeName);	}										\
::SE::Editor::TCustomEditorRegister<TypeName, ValueTypeName, Get##__LINE__##Name> __LINE__##Register;	\
