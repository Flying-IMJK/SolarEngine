#pragma once
#include "Editor/Core/PropertyGrid/PropertyGrid.h"
#include "Editor/Resource/Import/RawFileInspector.h"
#include "ResourceDescriptor.h"

//-------------------------------------------------------------------------

namespace SGE::Editor
{
    // Import Settings
    //-------------------------------------------------------------------------
    // Allows us to provide a user friendly mechanism for importing raw files into the engine
    // This is essentially a fancy descriptor editor

    class ImportSettings
    {
    public:

        ImportSettings( EditorContext const* pEditorContext );

        virtual ~ImportSettings() = default;

        // Get the name for the type of resource we're importing
        virtual char const* GetName() = 0;

        // Should these import settings be shown given the state of the descriptor
        virtual bool IsVisible() const = 0;

        // Update the descriptor state using a specified source file and a set of selected importable items
        void UpdateDescriptor( ResPath sourceFileResourcePath, List<Import::ImportableItem*> const& selectedItems );

        // Update and draw the settings - returns true if a new descriptor was created
        bool UpdateAndDraw( ResPath& outCreatedDescriptorPath );

    protected:

        // Update the descriptor
        virtual void UpdateDescriptorInternal( ResPath sourceFileResourcePath, List<Import::ImportableItem*> const& selectedItems ) = 0;

        // Do we have any extra import options
        virtual bool HasExtraOptions() const { return false; }

        // How much space do we need to display any extra options
        virtual float GetExtraOptionsRequiredHeight() const { return 0; }

        // Draw extra options
        virtual void DrawExtraOptions() {}

        // Get the descriptor
        virtual ResourceDescriptor const* GetDescriptor() const = 0;

        // Opportunity to run any operations just before we save the descriptor - returns false if something fails
        virtual bool PreSaveDescriptor( String const descriptorSavePath ) { return true; }

        // Always runs after we attempt to save a descriptor.
        virtual void PostSaveDescriptor( String const descriptorSavePath ) {}

        // Get the descriptor
        inline ResourceDescriptor* GetDescriptor() { return const_cast<ResourceDescriptor*>( const_cast<ImportSettings const*>( this )->GetDescriptor() ); }

        // Create the default resource path for the currently set source path (i.e. same path as source with an extension change)
        void GenerateDefaultDescriptorPath();

        // Ask the user to provide a path for the descriptor
        bool TryGetUserSpecifiedDescriptorPath( String& outPath ) const;

        // Save the descriptor in the specified path
        bool TrySaveDescriptorToDisk( String const filePath );

    protected:

        EditorContext const*                           m_pEditorContext = nullptr;
        PropertyGrid                                   m_propertyGrid;
        ResPath                                        m_sourceResourcePath;
        ResPath                                        m_defaultDescriptorResourcePath;
    };

    // Derived class binding import settings to a concrete descriptor
    //-------------------------------------------------------------------------

    template<typename T>
    class TImportSettings : public ImportSettings
    {
    public:

        TImportSettings( EditorContext const* pEditorContext ) 
            : ImportSettings( pEditorContext )
        {
            m_propertyGrid.SetTypeToEdit( &m_descriptor );
        }

    protected:

        virtual ResourceDescriptor const* GetDescriptor() const override { return &m_descriptor; }

    protected:

        T                                                   m_descriptor;
    };
}