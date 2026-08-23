#include "ResourceImportSettings.h"
#include "Editor/EditorContext.h"
#include "Editor/Core/CommonDialogs.h"

#include "Core/Platform/FileSystem.h"

//-------------------------------------------------------------------------

namespace SGE::Editor
{
    ImportSettings::ImportSettings( EditorContext const* pEditorContext )
        : m_pEditorContext( pEditorContext )
        , m_propertyGrid( pEditorContext )
    {}

    bool ImportSettings::UpdateAndDraw( ResPath& outCreatedDescriptorPath )
    {
        ENGINE_ASSERT( IsVisible() );

        outCreatedDescriptorPath.Clear();

        //-------------------------------------------------------------------------

        auto const& style = ImGui::GetStyle();
        ImVec2 const availableSpaceSize = ImGui::GetContentRegionAvail();
        float const requiredExtraOptionsHeight = HasExtraOptions() ? GetExtraOptionsRequiredHeight() : 0;
        constexpr static float const buttonRowHeight = 30;

        // Property Grid
        //-------------------------------------------------------------------------

        ImVec2 propertyGridWindowSize( availableSpaceSize.x, availableSpaceSize.y - requiredExtraOptionsHeight - buttonRowHeight - ( style.ItemSpacing.y * 2 ) );
        if ( ImGui::BeginChild( "grid", propertyGridWindowSize, 0, 0 ) )
        {
            m_propertyGrid.DrawGrid();
        }
        ImGui::EndChild();

        // Extra Controls
        //-------------------------------------------------------------------------

        if ( HasExtraOptions() )
        {
            ImVec2 extraOptionsWindowSize( availableSpaceSize.x, requiredExtraOptionsHeight );
            if ( ImGui::BeginChild( "extraOptions", extraOptionsWindowSize, 0, 0 ) )
            {
                DrawExtraOptions();
            }
            ImGui::EndChild();
        }

        // Button Row
        //-------------------------------------------------------------------------

        bool wasDescriptorSuccessfullyCreated = false;

        ResourceDescriptor const* pDescriptor = GetDescriptor();

		String const defaultDescriptorFilePath = m_defaultDescriptorResourcePath.IsValid() ? m_defaultDescriptorResourcePath.ToFileSystemPath( m_pEditorContext->GetRawResourceDirectory() ) : String();

        ImGui::BeginDisabled( !pDescriptor->IsValid() || defaultDescriptorFilePath.IsEmpty() || FileSystem::FileExists(defaultDescriptorFilePath));
        if ( SGUI::ColoredIconButton( Colors::Green, Colors::White, Colors::White, StringView(ICON_FILE_IMPORT), SE_TEXT("Import"), ImVec2( 100, 0 ) ) )
        {
            if ( TrySaveDescriptorToDisk( defaultDescriptorFilePath ) )
            {
                outCreatedDescriptorPath = ResPath::FromFileSystemPath( m_pEditorContext->GetRawResourceDirectory(), defaultDescriptorFilePath );
                wasDescriptorSuccessfullyCreated = true;
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled( !pDescriptor->IsValid() || !m_defaultDescriptorResourcePath.IsValid() );
        if ( SGUI::ColoredIconButton( Colors::Green, Colors::White, Colors::White, StringView(ICON_FILE_IMPORT) , SE_TEXT("Import As"), Float2( 100, 0 ) ) )
        {
            String userSpecifiedDescriptorFilePath;
            if ( TryGetUserSpecifiedDescriptorPath( userSpecifiedDescriptorFilePath ) )
            {
                if ( TrySaveDescriptorToDisk( userSpecifiedDescriptorFilePath ) )
                {
                    outCreatedDescriptorPath = ResPath::FromFileSystemPath( m_pEditorContext->GetRawResourceDirectory(), userSpecifiedDescriptorFilePath );
                    wasDescriptorSuccessfullyCreated = true;
                }
            }
        }
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

        return wasDescriptorSuccessfullyCreated;
    }

    void ImportSettings::UpdateDescriptor( ResPath sourceFileResourcePath, List<Import::ImportableItem*> const& selectedItems )
    {
        m_sourceResourcePath = sourceFileResourcePath;
        GenerateDefaultDescriptorPath();
        UpdateDescriptorInternal( sourceFileResourcePath, selectedItems );
    }

    void ImportSettings::GenerateDefaultDescriptorPath()
    {
        if ( m_sourceResourcePath.IsValid() && m_sourceResourcePath.IsFile() )
        {
            ResourceDescriptor const* pDesc = GetDescriptor();
			TypeID const resourceTypeID = pDesc->GetCompiledResourceTypeID();
            ENGINE_ASSERT( resourceTypeID.IsValid() );

            //-------------------------------------------------------------------------

            String extension = resourceTypeID.ToString();
			extension = extension.ToLower();
			
            m_defaultDescriptorResourcePath = m_sourceResourcePath;
            m_defaultDescriptorResourcePath.ReplaceExtension( extension );
        }
        else
        {
            m_defaultDescriptorResourcePath.Clear();
        }
    }

    bool ImportSettings::TryGetUserSpecifiedDescriptorPath( String& outPath ) const
    {
        ResourceDescriptor const* pDesc = GetDescriptor();
		TypeID const resourceTypeID = pDesc->GetCompiledResourceTypeID();
        ENGINE_ASSERT( resourceTypeID.IsValid() );

        ENGINE_ASSERT( m_defaultDescriptorResourcePath.IsValid() );
        String const newDescriptorPath = m_defaultDescriptorResourcePath.ToFileSystemPath( m_pEditorContext->GetRawResourceDirectory() );

//        ResourceInfo const* pResourceInfo = Types::GetResourceInfoForResourceType(resourceTypeID );
        TypeInfo const* pResourceInfo = Types::GetTypeInfo(resourceTypeID );
        return SaveDialog( resourceTypeID, outPath, newDescriptorPath, pResourceInfo->friendlyName);
    }

    bool ImportSettings::TrySaveDescriptorToDisk( String const filePath )
    {
        ENGINE_ASSERT( !filePath.IsEmpty() );

        ResourceDescriptor const* pDescriptor = GetDescriptor();
        ENGINE_ASSERT( pDescriptor != nullptr );

        ResID resourceID = ResPath::FromFileSystemPath( m_pEditorContext->GetRawResourceDirectory(), filePath );
        ENGINE_ASSERT( resourceID.GetResourceTypeID() == pDescriptor->GetCompiledResourceTypeID() );

        //-------------------------------------------------------------------------

        bool result = true;

        // Run any require pre-save operations
        // This may further mutate the descriptor
        if ( !PreSaveDescriptor( filePath ) )
        {
            result = false;
        }

        // Save the descriptor
        if ( result )
        {
/*
            Serialization::TypeArchiveWriter typeWriter;
            typeWriter << pDescriptor;
            if ( !typeWriter.WriteToFile( filePath ) )
            {
                pfd::message( "Failed to save descriptor!", "Failed to write descriptor to disk!", pfd::choice::ok, pfd::icon::error ).result();
                result = false;
            }
*/

        }

        // Always run post save to undo any changes we might have done in the pre-save
        PostSaveDescriptor( filePath );

        return result;
    }
}
