#pragma once

#include "Editor/API.h"
#include "Core/Utilities/GlobalRegistryBase.h"
#include "Core/TypeSystem/IType.h"

//-------------------------------------------------------------------------

namespace SE
{
    class StringID;

    namespace Editor
    {
        class EditorContext;
    }
    
}

//-------------------------------------------------------------------------
// Type Editing Rules
//-------------------------------------------------------------------------
// These classes provide additional editing rules/guidelines for the property grid to use when editing a given type

namespace SE::Editor::PG
{
    class SE_API_EDITOR TypeEditingRules
    {
    public:

        TypeEditingRules() = default;
        TypeEditingRules( TypeEditingRules const& ) = default;
        virtual ~TypeEditingRules() = default;

        TypeEditingRules& operator=( TypeEditingRules const& rhs ) = default;

    public:

        virtual bool IsReadOnly( StringID const& propertyID ) = 0;
        virtual bool IsHidden( StringID const& propertyID ) = 0;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class SE_API_EDITOR TTypeEditingRules : public TypeEditingRules
    {
        static_assert( TIsBaseOf<::SE::IType, T>::Value, "T is not derived from IReflectedType" );

    public:

        TTypeEditingRules( EditorContext const* pEditorContext, T* pTypeInstance )
            : m_pTypeInstance( pTypeInstance )
        {}

    protected:

        T* m_pTypeInstance = nullptr;
    };

    //-------------------------------------------------------------------------
    // Factory
    //-------------------------------------------------------------------------

    class SE_API_EDITOR TypeEditingRulesFactory : public GlobalRegistryBase<TypeEditingRulesFactory>
    {
        ENGINE_DECLARE_GLOBAL_REGISTRY(TypeEditingRulesFactory);

    public:

        virtual ~TypeEditingRulesFactory() = default;

        static TypeEditingRules* TryCreateRules( EditorContext const* pEditorContext, IType* pTypeInstance );

    protected:

        // Get the type that that this factory can create an helper for
        virtual TypeID GetSupportedTypeID() const = 0;

        // Virtual method that will create a helper if the type ID matches the appropriate type
        virtual TypeEditingRules* TryCreateRulesInternal( EditorContext const* pEditorContext, IType* pTypeInstance ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., EE_PROPERTY_GRID_EDITING_RULES( ObjectSettingsHelperFactory, ObjectSettings );

    #define ENGINE_PROPERTY_GRID_EDITING_RULES( factoryName, editedType, rulesClass )\
    class factoryName final : public PG::TypeEditingRulesFactory\
    {\
        virtual TypeID GetSupportedTypeID() const override { return editedType::GetStaticTypeID(); }\
        virtual PG::TypeEditingRules* TryCreateRulesInternal( EditorContext const* pEditorContext, IReflectedType* pTypeInstance ) const override\
        {\
            ENGINE_ASSERT( pTypeInstance != nullptr );\
            ENGINE_ASSERT( IsOfType<editedType>( pTypeInstance ) );\
            return New<rulesClass>( pEditorContext, static_cast<editedType*>( pTypeInstance ) );\
        }\
    };\
    static factoryName g_##factoryName;

}