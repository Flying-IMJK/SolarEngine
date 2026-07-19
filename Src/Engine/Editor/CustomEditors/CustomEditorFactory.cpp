#include "CustomEditorFactory.h"
#include "CustomEditor.h"
#include "Values/ValueContainer.h"
#include "Editors/GenericEditor.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/TypeSystem/Info/TypeCompositeInfo.h"
#include "Runtime/Core/TypeSystem/Property/TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/CustomEditorAliasAttribute.h"

namespace SE::Editor
{
	Dictionary<StableID, CustomEditorRegister*> CustomEditorFactory::s_editorFactories;
	Dictionary<TypeID, TypeID> CustomEditorFactory::s_editorTypeCache;

	CustomEditor* CustomEditorFactory::CreateEditor(TypeID type, bool canUseGeneric)
	{
		if (type == TypeID::Invalid)
		{
			LOG_WARNING("Editor", "CustomEditorFactory::CreateEditor - Invalid type");
			return canUseGeneric ? New<GenericEditor>() : nullptr;
		}

		// Check type cache first
		/*if (s_editorTypeCache.ContainsKey(type))
		{
			TypeID editorType = s_editorTypeCache[type];
			if (s_editorFactories.ContainsKey(editorType))
			{
				try
				{
					return s_editorFactories[editorType]->Create();
				}
				catch (const std::exception& e)
				{
					LOG_ERROR("Editor", "Failed to create custom editor from cache: {}", StringAnsiView(e.what()));
				}
			}
		}*/

		// Check if editor is directly registered for this type
		/*if (s_editorFactories.ContainsKey(type))
		{
			try
			{
				CustomEditor* editor = s_editorFactories[type]->Create();
				// Cache the result
				s_editorTypeCache[type] = type;
				return editor;
			}
			catch (const std::exception& e)
			{
				LOG_ERROR("Editor", "Failed to create custom editor: {}", StringAnsiView(e.what()));
			}
		}*/

		// Search base classes for registered editors
		const TypeCompositeInfo* typeInfo = Types::GetTypeInfo(type);
		if (typeInfo)
		{
			// Check base classes
			/*const TypeCompositeInfo* baseClasses = typeInfo->pParentTypeInfo;
			if (s_editorFactories.ContainsKey(baseClasses->id))
			{
				try
				{
					CustomEditor* editor = s_editorFactories[baseClasses->id]->Create();
					// Cache the result
					s_editorTypeCache[type] = baseClasses->id;
					return editor;
				}
				catch (const std::exception& e)
				{
					LOG_ERROR("Editor", "Failed to create custom editor from base class: {}", StringAnsiView(e.what()));
				}
			}*/
		}

		// Use generic editor as fallback
		if (canUseGeneric)
		{
			return New<GenericEditor>();
		}

		LOG_WARNING("Editor", "No custom editor found for type: {}", type);
		return nullptr;
	}

	CustomEditor* CustomEditorFactory::CreateEditor(ValueContainer* values, CustomEditor* overrideEditor)
	{
		if (!values)
		{
			LOG_ERROR("Editor", "CustomEditorFactory::CreateEditor - Invalid values container");
			return nullptr;
		}

		// Use override editor if provided
		if (overrideEditor)
		{
			return overrideEditor;
		}

		// Check for CustomEditorAttribute or CustomEditorAliasAttribute on the property
		ValueContent valueContent = values->At(0);
		/*const TypeProperty* info = values[0] ->GetInfo();
		if (info && info->metaContainer)
		{
			// Check for CustomEditorAttribute
			auto* customEditorAttr = info->metaContainer->Find<CustomEditorAttribute>();
			if (customEditorAttr && customEditorAttr->EditorType != TypeID::Invalid)
			{
				CustomEditor* editor = CreateEditor(customEditorAttr->EditorType, false);
				if (editor)
				{
					return editor;
				}
			}

			// Check for CustomEditorAliasAttribute
			auto* aliasAttr = info->metaContainer->Find<CustomEditorAliasAttribute>();
			if (aliasAttr && aliasAttr->typeID != TypeID::Invalid)
			{
				TypeID editorType = aliasAttr->typeID;
				if (editorType != TypeID::Invalid)
				{
					CustomEditor* editor = CreateEditor(editorType, false);
					if (editor)
					{
						return editor;
					}
				}
			}
		}*/

		// Use type-based editor creation
		TypeID valueType = valueContent.GetType();
		return CreateEditor(valueType, true);
	}

	void CustomEditorFactory::UnregisterEditor(TypeID type)
	{
		if (type == TypeID::Invalid)
		{
			LOG_ERROR("Editor", "CustomEditorFactory::UnregisterEditor - Invalid type");
			return;
		}

		/*if (s_editorFactories.ContainsKey(type))
		{
			s_editorFactories.Remove(type);

			// Clear cache entries
			List<TypeID> keysToRemove;
			for (const auto& pair : s_editorTypeCache)
			{
				if (pair.Value == type)
				{
					keysToRemove.Add(pair.Key);
				}
			}
			for (TypeID key : keysToRemove)
			{
				s_editorTypeCache.Remove(key);
			}

			LOG_INFO("Editor", "Unregistered custom editor for type: {}", type);
		}*/
	}

	bool CustomEditorFactory::HasCustomEditor(TypeID type)
	{
		if (type == TypeID::Invalid)
		{
			return false;
		}

		// Check direct registration
		/*if (s_editorFactories.ContainsKey(type))
		{
			return true;
		}

		// Check base classes
		const TypeCompositeInfo* typeInfo = Types::GetTypeInfo(type);
		if (typeInfo)
		{
			const TypeCompositeInfo* baseClasses = typeInfo->pParentTypeInfo;
			if (s_editorFactories.ContainsKey(baseClasses->id))
			{
				return true;
			}
		}*/

		return false;
	}

	TypeID CustomEditorFactory::GetEditorType(TypeID type)
	{
		if (type == TypeID::Invalid)
		{
			return TypeID::Invalid;
		}

		// Check cache
		/*if (s_editorTypeCache.ContainsKey(type))
		{
			return s_editorTypeCache[type];
		}

		// Check direct registration
		if (s_editorFactories.ContainsKey(type))
		{
			s_editorTypeCache[type] = type;
			return type;
		}

		// Check base classes
		const TypeCompositeInfo* typeInfo = Types::GetTypeInfo(type);
		if (typeInfo)
		{
			const TypeCompositeInfo* baseClasses = typeInfo->pParentTypeInfo;
			if (s_editorFactories.ContainsKey(baseClasses->id))
			{
				s_editorTypeCache[type] = baseClasses->id;
				return baseClasses->id;
			}
		}*/

		return TypeID::Invalid;
	}

	void CustomEditorFactory::RegisterEditor(CustomEditorRegister* editorRegister)
	{
		if (editorRegister == nullptr)
		{
			LOG_ERROR("Editor", "CustomEditorFactory::RegisterEditor - Invalid register");
			return;
		}

		StableID stableId = editorRegister->ValueTypeStableID();

		if (s_editorFactories.ContainsKey(stableId))
		{
			LOG_ERROR("Editor", "CustomEditorFactory::RegisterEditor - repetition register");
			return;
		}

		// Register the factory
		s_editorFactories[stableId] = editorRegister;

	}

	CustomEditorRegister::CustomEditorRegister(CustomEditorRegister* pRegister) : getNameFunc(nullptr)
	{
		CustomEditorFactory::RegisterEditor(pRegister);
	}

	StringView CustomEditorRegister::GetName() const
	{
		return StringView(getNameFunc());
	}
} // namespace SE::Editor