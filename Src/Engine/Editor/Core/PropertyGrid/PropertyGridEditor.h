#pragma once

#include "Editor/API.h"
#include "Editor/EditorContext.h"
#include "Core/TypeSystem/CoreTypes.h"
#include "Core/Utilities/GlobalRegistryBase.h"

//-------------------------------------------------------------------------

namespace SE
{
    class TypeProperty;

    namespace Resource { class ResourcePicker; }
}

//-------------------------------------------------------------------------

namespace SE::Editor::PG
{
    struct PropertyEditorContext
    {
        EditorContext const*     m_pEditorContext = nullptr;
        void*                   m_pUserContext = nullptr; // Additional context provided to the property grid for specialized use cases
    };

    //-------------------------------------------------------------------------

    class SE_API_EDITOR PropertyEditor
    {

    public:

        PropertyEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* pPropertyInstance );
        virtual ~PropertyEditor() = default;

        // 如果值已更新，则返回 true
        bool UpdateAndDraw();

        // Actually set the real property value from the working copy
        virtual void UpdatePropertyValue() = 0;

        // 将缓存的值重置为实际实例的值
        virtual void ResetWorkingCopy() = 0;

    protected:

        // 检查属性值是否与缓存的值不同，如果不同，则更新缓存的值
        virtual void HandleExternalUpdate() = 0;

        // 绘制编辑器小部件并处理用户输入，如果值已更新，则返回 true
        virtual bool InternalUpdateAndDraw() = 0;

    protected:
        PropertyEditorContext            m_context;
        TypeProperty const&     m_propertyInfo;
        void*                            m_pPropertyInstance;
        TypeIDCore const        m_coreType;
    };

    //-------------------------------------------------------------------------
    // Property Grid Editor Factory
    //-------------------------------------------------------------------------

    class SE_API_EDITOR PropertyGridEditorFactory : public GlobalRegistryBase<PropertyGridEditorFactory>
    {
        ENGINE_DECLARE_GLOBAL_REGISTRY( PropertyGridEditorFactory );

    public:

        virtual ~PropertyGridEditorFactory() = default;

        static PropertyEditor* TryCreateEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* pPropertyInstance );

    protected:

        // Does this factory support this type
        virtual bool SupportsTypeID( TypeID typeID ) const = 0;

        // Get the type that that this factory can create an editor for
        virtual bool SupportsCustomEditorID( StringID customEditorID ) const = 0;

        // Virtual method that will create a editor if the property type ID matches the appropriate types
        virtual PropertyEditor* TryCreateEditorInternal( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* pPropertyInstance ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a property grid editor factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., PROPERTY_GRID_EDITOR( ObjectSettingsEditorFactory, ObjectSettings, ObjectSettingsEditor );

    #define EE_PROPERTY_GRID_TYPE_EDITOR( factoryName, editedType, propertyGridEditorClass )\
    class factoryName final : public PG::PropertyGridEditorFactory\
    {\
        virtual bool SupportsTypeID( TypeID typeID ) const override { return editedType::GetStaticTypeID() == typeID; }\
        virtual bool SupportsCustomEditorID( StringID customEditorID ) const override { return false; }\
        virtual PG::PropertyEditor* TryCreateEditorInternal( PG::PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* pPropertyInstance ) const override\
        {\
            ENGINE_ASSERT( propertyInfo.IsValid() );\
            ENGINE_ASSERT( pPropertyInstance != nullptr );\
            return EE::New<propertyGridEditorClass>( context, propertyInfo, pPropertyInstance );\
        }\
    };\
    static factoryName g_##factoryName;

    #define PROPERTY_GRID_CUSTOM_EDITOR( factoryName, customEditorID, propertyGridEditorClass )\
    class factoryName final : public PG::PropertyGridEditorFactory\
    {\
        virtual bool SupportsTypeID( TypeID typeID ) const override { return false; }\
        virtual bool SupportsCustomEditorID( StringID editorID ) const override { static StringID const supportedEditorID( customEditorID ); return editorID == supportedEditorID; }\
        virtual PG::PropertyEditor* TryCreateEditorInternal( PG::PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* pPropertyInstance ) const override\
        {\
            ENGINE_ASSERT( propertyInfo.IsValid() );\
            ENGINE_ASSERT( pPropertyInstance != nullptr );\
            return EE::New<propertyGridEditorClass>( context, propertyInfo, pPropertyInstance );\
        }\
    };\
    static factoryName g_##factoryName;
}