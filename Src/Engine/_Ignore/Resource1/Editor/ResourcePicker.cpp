#include "ResourcePicker.h"
#include "ResourceDatabase.h"
#include "ResourceToolDefines.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Runtime/Resource/IResource.h"

#include "Editor/EditorContext.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Platform/Platform.h"

//-------------------------------------------------------------------------

namespace SE::Editor
{
    static ImVec2 const g_buttonSize( 30, 0 );

    //-------------------------------------------------------------------------

    ResourcePicker::ResourcePicker(EditorContext const& toolsContext, TypeID resourceTypeID, ResID  const& resourceID)
        : m_toolsContext( toolsContext )
        , m_resourceTypeID( resourceTypeID )
        , m_resourceID( resourceID )
    {
        GenerateResourceOptionsList();
        GenerateFilteredOptionList();
    }

    bool ResourcePicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Validation
        //-------------------------------------------------------------------------

		TypeID actualResourceTypeID = m_resourceTypeID;
        bool validPath = true;

        if ( m_resourceID.IsValid() )
        {
            // Check resource TypeID
            //-------------------------------------------------------------------------

            // Get actual resource typeID
			/*auto const pExtension = m_resourceID.GetResourcePath().GetExtension();
			if ( pExtension != nullptr )
			{
				actualResourceTypeID = ResTypeID( pExtension );
			}
*/
            //-------------------------------------------------------------------------

/*            if ( m_resourceTypeID.IsValid() )
            {
                if ( !Types::IsTypeDerivedFrom(actualResourceTypeID, m_resourceTypeID ) )
                {
                    validPath = false;
                }
            }
            else
            {
                if ( !Types::IsTypeDerivedFrom(actualResourceTypeID ) )
                {
                    validPath = false;
                }
            }*/

            // Check if file exist
            //-------------------------------------------------------------------------

/*            if ( validPath )
            {
                validPath = m_toolsContext.pResourceDatabase->DoesResourceExist(m_resourceID );
            }*/
        }

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( -1, ImGui::GetFrameHeight() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            auto const& style = ImGui::GetStyle();
            float const itemSpacingX = style.ItemSpacing.x;

            // Combo Selector
            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !m_toolsContext.pResourceDatabase->IsDescriptorCacheBuilt() );
            {
                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 2 ) + ( g_buttonSize.x * 2 );

                // Type and path fields
                //-------------------------------------------------------------------------

                float typeStrWidth = 0;
                if ( actualResourceTypeID.IsValid() )
                {
                    float const startCursorPosX = ImGui::GetCursorPosX();
                    String const resourceTypeStr = String::Format(SE_TEXT("[{}]"), actualResourceTypeID.ToString());
                    ImGui::AlignTextToFramePadding();
//                    ImGui::TextColored( Colors::LightPink.ToFloat4(), resourceTypeStr.Get() );
                    ImGui::SameLine();

                    typeStrWidth = ImGui::GetCursorPosX() - startCursorPosX;
                }

                float const comboArrowWidth = ImGui::GetFrameHeight();
                float const totalPathWidgetWidth = contentRegionAvailableX - usedWidth - typeStrWidth;
                float const textWidgetWidth = totalPathWidgetWidth - comboArrowWidth - style.ItemSpacing.x;

                ImVec4 const pathColor = ( validPath ? GUIStyle::s_colorText : Colors::Red ).ToFloat4();
                ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
                ImGui::SetNextItemWidth( textWidgetWidth );
                String const& pathString = m_resourceID.ToString();
//                ImGui::InputText( "##ResourcePathText", const_cast<char*>( pathString.Get() ), pathString.Length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TryUpdateResourceFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }

                // Tooltip
                if ( m_resourceID.IsValid() )
                {
                    ImGui::ItemTooltip( m_resourceID.c_str() );
                }

                // Allow pasting valid paths
                if ( ImGui::IsItemFocused() )
                {
                    if ( ImGui::IsKeyDown( ImGuiMod_Shortcut ) && ImGui::IsKeyPressed( ImGuiKey_V ) )
                    {
                        valueUpdated |= TryUpdateResourceFromClipboard();
                    }
                }

                // Combo
                //-------------------------------------------------------------------------

                ImVec2 const comboDropDownSize( Math::Max( totalPathWidgetWidth, 500.0f ), ImGui::GetFrameHeight() * 20 );
                ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

                ImGui::SameLine();
                bool const wasComboOpen = m_isComboOpen;
                m_isComboOpen = ImGui::BeginCombo( "##DataPath", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview );

                // Regenerate options and clear filter each time we open the combo
                if ( m_isComboOpen && !wasComboOpen )
                {
                    m_filterWidget.Clear();
                    GenerateResourceOptionsList();
                    GenerateFilteredOptionList();
                }

                // Draw combo if open
                bool shouldUpdateNavID = false;
                if ( m_isComboOpen )
                {
                    float const cursorPosYPreFilter = ImGui::GetCursorPosY();
                    if ( m_filterWidget.UpdateAndDraw( -1, ImGui::FilterWidget::Flags::TakeInitialFocus ) )
                    {
                        GenerateFilteredOptionList();
                    }
                    float const cursorPosYPostFilter = ImGui::GetCursorPosY();
                    float const filterHeight = cursorPosYPostFilter;

                    ImVec2 const previousCursorPos = ImGui::GetCursorPos();
                    ImVec2 const childSize( ImGui::GetContentRegionAvail().x, comboDropDownSize.y - filterHeight - style.ItemSpacing.y - style.WindowPadding.y );
                    ImGui::Dummy( childSize );
                    ImGui::SetCursorPos( previousCursorPos );

                    //-------------------------------------------------------------------------

                    if ( ImGui::BeginChild( "##ResList", childSize, false, ImGuiWindowFlags_NavFlattened ) )
                    {
                        GUI::ScopedFont const sfo( GUI::Font::Medium );

                        ImGuiListClipper clipper;
                        clipper.Begin( m_filteredResourceIDs.Count() );
                        while ( clipper.Step() )
                        {
                            for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                            {
/*                                if ( ImGui::Selectable( m_filteredResourceIDs[i].c_str() + 7 ) )
                                {
                                    m_resourceID = m_filteredResourceIDs[i];
                                    valueUpdated = true;
                                    ImGui::CloseCurrentPopup();
                                }*/
                                ImGui::ItemTooltip( m_filteredResourceIDs[i].c_str() );
                            }
                        }
                        clipper.End();
                    }
                    ImGui::EndChild();

                    ImGui::EndCombo();
                }
            }
            ImGui::EndDisabled();

            // Buttons
            //-------------------------------------------------------------------------

            {
                ImGui::BeginDisabled( !validPath );

                // Open Resource
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( ICON_OPEN_IN_NEW "##Open", g_buttonSize ) )
                {
                    m_toolsContext.TryOpenResource( m_resourceID );
                }
                ImGui::ItemTooltip(SE_TEXT("Open Resource"));

                // Options
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button(ICON_COG "##Options", g_buttonSize ) )
                {
                    ImGui::OpenPopup( "##ResourcePickerOptions" );
                }
                ImGui::ItemTooltip(SE_TEXT("Options"));

                ImGui::EndDisabled();
            }

            // Options Context Menu
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopup( "##ResourcePickerOptions" ) )
            {
                if ( ImGui::MenuItem( ICON_FILE_OUTLINE " Copy Resource Path" ) )
                {
//                    ImGui::SetClipboardText( m_resourceID.c_str() );
                }

                if ( ImGui::MenuItem( ICON_FOLDER_OPEN_OUTLINE " Show In Resource Browser" ) )
                {
                    m_toolsContext.TryFindInResourceBrowser( m_resourceID );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( ICON_FILE " Copy File Path" ) )
                {
                    String const fileSystemPath = m_resourceID.ToFileSystemPath( m_toolsContext.GetRawResourceDirectory() );
//                    ImGui::SetClipboardText( fileSystemPath.c_str() );
                }

                if ( ImGui::MenuItem( ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
					String const fileSystemPath = m_resourceID.ToFileSystemPath( m_toolsContext.GetRawResourceDirectory() );
                    FileSystem::ShowFileExplorer(fileSystemPath );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( ICON_ERASER " Clear" ) )
                {
                    m_resourceID.Clear();
                    valueUpdated = true;
                }

                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        //-------------------------------------------------------------------------

        ImGui::PopID();

        return valueUpdated;
    }

    void ResourcePicker::GenerateResourceOptionsList()
    {
        m_allResourceIDs.Clear();

        // Restrict options to specified resource type ID
        if ( m_resourceTypeID.IsValid() )
        {
            ENGINE_ASSERT(Types::IsTypeDerivedFrom(m_resourceTypeID, Typeof<IResource>()));

/*            for ( auto const& resourceID : m_toolsContext.pResourceDatabase->GetAllResourcesOfType(m_resourceTypeID ) )
            {
                m_allResourceIDs.Add( resourceID );
            }*/
        }
        else // All resource options are valid
        {
/*            for ( auto const& resourceListPair : m_toolsContext.pResourceDatabase->GetAllResources() )
            {
                for ( auto const& resourceRecord : resourceListPair.Value )
                {
                    ENGINE_ASSERT( resourceRecord->resourceID.IsValid() );
                    m_allResourceIDs.Add( resourceRecord->resourceID );
                }
            }*/
        }
    }

    void ResourcePicker::GenerateFilteredOptionList()
    {
        if ( m_filterWidget.HasFilterSet() )
        {
            m_filteredResourceIDs.Clear();

            for ( auto const& resourceID : m_allResourceIDs )
            {
                String lowercasePath = resourceID.GetResourcePath().GetString().ToLower();

                if ( m_filterWidget.MatchesFilter( lowercasePath ) )
                {
                    m_filteredResourceIDs.Add( resourceID );
                }
            }
        }
        else
        {
            m_filteredResourceIDs = m_allResourceIDs;
        }
    }

    void ResourcePicker::SetRequiredResourceType( TypeID resourceTypeID )
    {
        m_resourceTypeID = resourceTypeID;
        GenerateResourceOptionsList();
        GenerateFilteredOptionList();
    }

    void ResourcePicker::SetResourceID( ResID  const& resourceID )
    {
        if ( resourceID.IsValid() && m_resourceTypeID.IsValid() )
        {
            ENGINE_ASSERT( resourceID.GetResourceTypeID() == m_resourceTypeID );
        }

        m_resourceID = resourceID;
    }

    bool ResourcePicker::TryUpdateResourceFromClipboard()
    {
        String clipboardText = ImGui::GetClipboardText();

        // Check for a valid file path if the resource path is bad
        //-------------------------------------------------------------------------

        ResPath resourcePath;

        if ( !ResPath::IsValidPath( clipboardText ) )
        {
            String pastedFilePath( clipboardText );
            if ( !pastedFilePath.IsEmpty() && FileSystem::IsUnderDirectory(pastedFilePath ,m_toolsContext.GetRawResourceDirectory()))
            {
                resourcePath = ResPath::FromFileSystemPath( m_toolsContext.GetRawResourceDirectory(), pastedFilePath );
            }
        }
        else
        {
            resourcePath = ResPath( clipboardText );
        }

        if ( !resourcePath.IsValid() )
        {
            return false;
        }

        // Validate resource
        //-------------------------------------------------------------------------

        ResID  potentialNewResourceID( resourcePath );

        // Validate resource type
        if ( m_resourceTypeID.IsValid() )
        {
            if ( potentialNewResourceID.GetResourceTypeID() != m_resourceTypeID )
            {
                return false;
            }
        }

        m_resourceID = potentialNewResourceID;
        return true;
    }

    bool ResourcePicker::TryUpdateResourceFromDragAndDrop()
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload(DragAndDrop::s_payloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                ResID  const droppedResourceID( (char*) payload->Data );
                if ( droppedResourceID.IsValid() && droppedResourceID.GetResourceTypeID() == m_resourceTypeID )
                {
                    m_resourceID = droppedResourceID;
                    return true;
                }
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    bool ResourcePathPicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        String filePath;
        bool validPath = false;
        if ( m_resourcePath.IsValid() )
        {
            filePath = m_resourcePath.ToFileSystemPath( m_rawResourceDirectoryPath );
            validPath = FileSystem::FileExists(filePath);
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", {-1, ImGui::GetFrameHeight() }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            float const itemSpacingX = ImGui::GetStyle().ItemSpacing.x;

            // Type and path
            //-------------------------------------------------------------------------

            {
                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 2 ) + ( g_buttonSize.x * 2 ) + 1;

                // Resource path
                ImGui::SetNextItemWidth( contentRegionAvailableX - usedWidth );
                String resourcePathStr = m_resourcePath.GetString();
                ImGui::PushStyleColor( ImGuiCol_Text, validPath ? GUIStyle::s_colorText : Colors::Red );
				if (SGUI::InputText( "##DataPath", resourcePathStr, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly ))
				{
					m_resourcePath = ResPath(resourcePathStr);
				}
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TrySetPathFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }

                // Allow pasting valid paths
                if ( ImGui::IsItemFocused() )
                {
                    if ( ImGui::IsKeyDown( ImGuiMod_Shortcut ) && ImGui::IsKeyPressed( ImGuiKey_V ) )
                    {
                        String clipboardText = ImGui::GetClipboardText();
                        if ( ResPath::IsValidPath( clipboardText ) )
                        {
                            m_resourcePath = ResPath( clipboardText );
                            valueUpdated = true;
                        }
                        else
                        {
                            String pastedFilePath( clipboardText );
                            if ( !pastedFilePath.IsEmpty() && FileSystem::IsUnderDirectory(pastedFilePath, m_rawResourceDirectoryPath ) )
                            {
                                m_resourcePath = ResPath::FromFileSystemPath( m_rawResourceDirectoryPath, pastedFilePath );
                                valueUpdated = true;
                            }
                        }
                    }
                }

                // Tooltip
                if ( m_resourcePath.IsValid() )
                {
                    SGUI::ItemTooltip( m_resourcePath.c_str() );
                }

                // Pick Path
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( ICON_FILE_SEARCH_OUTLINE "##Pick", g_buttonSize ) )
                {
					List<String> files;
                    if (FileSystem::ShowOpenFileDialog(m_rawResourceDirectoryPath,
						StringView::Empty, false, SE_TEXT("文件导入"), files) && !files.IsEmpty())
                    {
                        String const selectedPath(files[0]);

                        if (FileSystem::IsUnderDirectory(selectedPath, m_rawResourceDirectoryPath ) )
                        {
                            m_resourcePath = ResPath::FromFileSystemPath( m_rawResourceDirectoryPath, selectedPath );
                            valueUpdated = true;
                        }
                        else
                        {
							NOTIFY_ERROR(SE_TEXT("Selected file is not with the raw resource folder!"));
                        }
                    }
                }
                SGUI::ItemTooltip(SE_TEXT("Pick Resource"));
            }

            // Options
            //-------------------------------------------------------------------------

            {
                ImGui::SameLine( 0, itemSpacingX );
                ImGui::BeginDisabled( !m_resourcePath.IsValid() );
                if ( ImGui::Button( ICON_COG "##Options", g_buttonSize ) )
                {
                    ImGui::OpenPopup( "##ResourcePickerOptions" );
                }
                SGUI::ItemTooltip(SE_TEXT("Options"));
                ImGui::EndDisabled();
            }

            // Options Context Menu
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopup( "##ResourcePickerOptions" ) )
            {
                if ( ImGui::MenuItem( ICON_FILE_OUTLINE" Copy Resource Path" ) )
                {
//                    ImGui::SetClipboardText( m_resourcePath.c_str() );
                }

                if ( ImGui::MenuItem( ICON_FILE" Copy File Path" ) )
                {
//                    ImGui::SetClipboardText( filePath.c_str() );
                }

                if ( ImGui::MenuItem( ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
                    FileSystem::ShowFileExplorer(filePath);
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( ICON_ERASER " Clear" ) )
                {
                    m_resourcePath.Clear();
                    valueUpdated = true;
                }

                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
        ImGui::PopID();

        //-------------------------------------------------------------------------

        return valueUpdated;
    }

    bool ResourcePathPicker::TrySetPathFromDragAndDrop()
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload(DragAndDrop::s_payloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                ResID  const droppedResourceID( (char*) payload->Data );
                if ( droppedResourceID.IsValid() )
                {
                    m_resourcePath = droppedResourceID.GetResourcePath();
                    return true;
                }
            }
        }

        return false;
    }
}