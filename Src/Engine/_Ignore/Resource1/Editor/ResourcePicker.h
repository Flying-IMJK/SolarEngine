#pragma once
#include "Editor/API.h"
#include "Runtime/SGUI/Widgets/FilterWidget.h"
#include "Runtime/Resource/ResourceID.h"

#include "Core/Platform/FileSystem.h"
#include "Core/TypeSystem/TypeID.h"
//-------------------------------------------------------------------------

namespace SE::Editor
{
    class ResourceDatabase;
    class EditorContext;

    //-------------------------------------------------------------------------

    class SE_API_EDITOR ResourcePicker final
    {

    public:

        ResourcePicker(EditorContext const& toolsContext, TypeID resourceTypeID = TypeID(), ResID  const& resourceID = ResID ());

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the type of resource we wish to select
        void SetRequiredResourceType( TypeID resourceTypeID );

        // Set the path
        void SetResourceID( ResID  const& resourceID );

        // Get the resource path that was set
        inline ResID  const& GetResourceID() const { return m_resourceID; }

    private:

        // Generate the set of valid resource options
        void GenerateResourceOptionsList();

        // Generate the set of filtered options
        void GenerateFilteredOptionList();

        // Try to update the resourceID from a paste operation - returns true if the value was updated
        bool TryUpdateResourceFromClipboard();

        // Try to update the resourceID from a drag and drop operation - returns true if the value was updated
        bool TryUpdateResourceFromDragAndDrop();

    private:

        EditorContext const&        m_toolsContext;
		TypeID             m_resourceTypeID; // The type of resource we should pick from
        ResID                   	m_resourceID;
        ImGui::FilterWidget          m_filterWidget;
        List<ResID >          	m_allResourceIDs;
		List<ResID >          	m_filteredResourceIDs;
        bool                        m_isComboOpen = false;
    };

    //-------------------------------------------------------------------------

    class SE_API_EDITOR ResourcePathPicker final
    {
    public:

        ResourcePathPicker( String const& rawResourceDirectoryPath, ResPath const& path = ResPath() )
            : m_rawResourceDirectoryPath( rawResourceDirectoryPath )
            , m_resourcePath( path )
        { 
            ENGINE_ASSERT( !m_rawResourceDirectoryPath.IsEmpty() && FileSystem::DirectoryExists(m_rawResourceDirectoryPath));
        }

        // Update the widget and draws it, returns true if the path was updated
        bool UpdateAndDraw();

        // Set the path
        void SetPath( ResPath const& path ) { m_resourcePath = path; }

        // Get the resource path that was set
        inline ResPath const& GetPath() const { return m_resourcePath; }

        // Try to update the path from a drag and drop operation - returns true if the value was updated
        bool TrySetPathFromDragAndDrop();

    private:

        String const      		m_rawResourceDirectoryPath;
        ResPath                	m_resourcePath;
    };
}