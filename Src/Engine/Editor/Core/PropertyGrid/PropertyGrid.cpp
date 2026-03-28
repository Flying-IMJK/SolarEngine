#include "PropertyGrid.h"
#include "Editor/EditorContext.h"
#include "Core/TypeSystem/Types.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "PropertyGridEditor.h"
#include "PropertyGridTypeEditingRules.h"

//-------------------------------------------------------------------------

namespace SE::Editor::PG
{
    struct GridContext : public PropertyEditorContext
    {
        PropertyGrid*               m_pPropertyGrid = nullptr;
    };
}

//-------------------------------------------------------------------------

namespace SE::Editor
{
    static String GetFriendlyTypename( Types const& typeRegistry, TypeID typeID )
    {
        ENGINE_ASSERT( typeID != TypeID::Invalid );
        String friendlyTypeName;

        // Get core type friendly name
        TypeIDCore const coreTypeID = CoreTypeRegistry::GetType( typeID );
        if ( coreTypeID != TypeIDCore::Invalid )
        {
            friendlyTypeName = CoreTypeRegistry::GetFriendlyName( coreTypeID );
        }
        else // Read type info from type registry
        {
            auto pTypeInfo = typeRegistry.GetTypeInfo( typeID );
            ENGINE_ASSERT( pTypeInfo != nullptr );

            if ( pTypeInfo->HasCategoryName() )
            {
                friendlyTypeName = pTypeInfo->GetCategoryName().ToString() + SE_TEXT("/") + pTypeInfo->friendlyName;
            }
            else
            {
                friendlyTypeName = pTypeInfo->friendlyName;
            }
        }

        return friendlyTypeName;
    }

    //-------------------------------------------------------------------------

    PropertyGrid::PropertyGrid(EditorContext const* pEditorContext )
    {
        ENGINE_ASSERT(pEditorContext != nullptr && pEditorContext->IsValid() );

        m_pGridContext = New<PG::GridContext>();
        m_pGridContext->m_pPropertyGrid = this;
        m_pGridContext->m_pEditorContext = pEditorContext;
    }

    PropertyGrid::~PropertyGrid()
    {
        m_pTypeInstance = nullptr;
        m_pTypeInfo = nullptr;
        RebuildGrid();
        Delete( m_pGridContext );
    }

    void PropertyGrid::SetUserContext( void* pContext )
    {
        m_pGridContext->m_pUserContext = pContext;
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::SetTypeToEdit( IType* pTypeInstance )
    {
        if ( pTypeInstance == nullptr )
        {
            SetTypeToEdit( nullptr );
        }
        else if( pTypeInstance != m_pTypeInstance )
        {
            m_pTypeInfo = pTypeInstance->GetTypeInfo();
            m_pTypeInstance = pTypeInstance;
            m_isDirty = false;
            RebuildGrid();
        }
    }

    void PropertyGrid::SetTypeToEdit( nullptr_t )
    {
        m_pTypeInfo = nullptr;
        m_pTypeInstance = nullptr;
        m_isDirty = false;
        RebuildGrid();
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::RebuildGrid()
    {
        for ( auto& pCategory : m_categories )
        {
            pCategory->DestroyChildren();
            Delete( pCategory );
        }

        m_categories.Clear();

        //-------------------------------------------------------------------------

        if ( m_pTypeInfo == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------
        for (auto const& propertyInfo : m_pTypeInfo->properties)
        {
            PG::CategoryRow* pCategory = PG::CategoryRow::FindOrCreateCategory( nullptr, *m_pGridContext, 
				m_categories, propertyInfo->category.IsEmpty() ? SE_TEXT("General") : propertyInfo->category );
            pCategory->AddProperty( m_pTypeInstance, *propertyInfo );
        }

        //-------------------------------------------------------------------------

        ApplyFilter();
    }

    void PropertyGrid::ApplyFilter()
    {
        auto const& filters = m_filterWidget.GetFilterTokens();
        Function<void(PG::GridRow*)> EvaluateRowFilter = [this, &filters]( PG::GridRow* pRow )
        {
            // Reset Visibility
            pRow->SetHidden( false );

            // Apply read-only visibility filter
            if ( pRow->IsReadOnly() )
            {
                pRow->SetHidden( !m_showReadOnlyProperties );
            }

            // Apply name filter
            if ( !pRow->IsHidden() && !filters.IsEmpty() )
            {
                String rowNameLowercase = pRow->GetName();
                rowNameLowercase.ToLower();

                bool shouldRowBeHidden = true;
                for ( auto const& filter : filters )
                {
                    if ( rowNameLowercase.Find( filter ) != INVALID_INDEX)
                    {
                        shouldRowBeHidden = false;
                        break;
                    }
                }

                pRow->SetHidden( shouldRowBeHidden );
            }
        };

        for ( auto& pCategory : m_categories )
        {
            pCategory->RecursiveOperation(EvaluateRowFilter);
        }
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::ExpandAllPropertyViews()
    {
        for ( auto& pCategory : m_categories )
        {
            pCategory->SetExpansion( true );
        }
    }

    void PropertyGrid::CollapseAllPropertyViews()
    {
        for ( auto& pCategory : m_categories )
        {
            pCategory->SetExpansion( false );
        }
    }

    //-------------------------------------------------------------------------

    void PropertyGrid::DrawGrid( bool shouldFillRemainingSpace )
    {
        if ( m_pTypeInstance == nullptr )
        {
            ImGui::Text( "Nothing To Edit." );
            return;
        }

        //-------------------------------------------------------------------------

//        ImGui::BeginDisabled( m_isReadOnly );

        // Control Bar
        //-------------------------------------------------------------------------

        if ( m_isControlBarVisible )
        {
            constexpr float const buttonWidth = 30;
            float const filterWidth = ImGui::GetContentRegionAvail().x - ( 3 * ( buttonWidth + ImGui::GetStyle().ItemSpacing.x ) );
            if ( m_filterWidget.UpdateAndDraw( filterWidth ) )
            {
                ApplyFilter();
            }

            ImGui::SameLine();

            if (ImGui::Button( SE_TEXT(ICON_COLLAPSE_ALL"##CollapseAll"), ImVec2( buttonWidth, 0 ) ) )
            {
                CollapseAllPropertyViews();
            }
            ImGui::ItemTooltip(SE_TEXT("Collapse All"));

            ImGui::SameLine();

            if ( ImGui::Button(SE_TEXT(ICON_EXPAND_ALL"##ExpandAll")) )
            {
                ExpandAllPropertyViews();
            }
            ImGui::ItemTooltip( SE_TEXT("Expand All"));

            ImGui::SameLine();

            if ( ImGui::ToggleButton( ICON_EYE_OUTLINE"##ToggleShowEditableProperties", ICON_EYE_OFF_OUTLINE"##ToggleShowEditableProperties", m_showReadOnlyProperties ) )
            {
                ApplyFilter();
            }
            ImGui::ItemTooltip( SE_TEXT("Read-only property visibility"));
        }

        // Grid Rows
        //-------------------------------------------------------------------------

        ImVec2 const tableSize = ImVec2( ImGui::GetContentRegionAvail().x - 1, ( shouldFillRemainingSpace ? ImGui::GetContentRegionAvail().y : 0 ) - 1 );
        ImGuiTableFlags const flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 8 ) );
        if ( ImGui::BeginTable( "GridTable", 2, flags, tableSize ) )
        {
            ImGui::TableSetupColumn( "##Header", ImGuiTableColumnFlags_WidthFixed, 200 );
            ImGui::TableSetupColumn( "##Editor", ImGuiTableColumnFlags_WidthStretch );

            //-------------------------------------------------------------------------

            for ( auto& pCategory : m_categories )
            {
                pCategory->UpdateRow();
                pCategory->DrawRow( 0 );
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

//        ImGui::EndDisabled();
    }
}

//-------------------------------------------------------------------------

namespace SE::Editor::PG
{
    constexpr float const g_headerOffset = 20;
    static ImVec2 const g_controlButtonSize( 18, 24 );
    static ImVec2 const g_controlButtonPadding( 0, 4 );

    //-------------------------------------------------------------------------

    struct [[nodiscard]] ScopedChangeNotifier
    {
        ScopedChangeNotifier( PropertyGrid* pGrid, IType* pTypeInstance, TypeProperty const* pPropertyInfo, PropertyEditInfo::Action action = PropertyEditInfo::Action::Edit )
            : m_pGrid( pGrid )
        {
            ENGINE_ASSERT( pGrid != nullptr );
            ENGINE_ASSERT( pTypeInstance != nullptr );
            m_eventInfo.m_pOwnerTypeInstance = pGrid->m_pTypeInstance;
            m_eventInfo.m_pTypeInstanceEdited = pTypeInstance;
            m_eventInfo.m_pPropertyInfo = pPropertyInfo;
            m_eventInfo.m_action = action;
            m_pGrid->m_preEditEvent.Execute( m_eventInfo );
        }

        ~ScopedChangeNotifier()
        {
            // m_eventInfo.m_pTypeInstanceEdited->PostPropertyEdit( m_eventInfo.m_pPropertyInfo );
            m_pGrid->m_postEditEvent.Execute( m_eventInfo );
            m_pGrid->m_isDirty = true;
            m_pGrid->ApplyFilter();
        }

        PropertyGrid*                       m_pGrid = nullptr;
        PropertyEditInfo                    m_eventInfo;
    };

    //-------------------------------------------------------------------------

    static GridRow* CreateRow( GridRow* pParentRow, GridContext const& context, IType* pTypeInstance, TypeProperty const& propertyInfo, int32 arrayElementIdx = -1 )
    {
        GridRow* pRow = nullptr;

        if ( propertyInfo.IsArrayProperty() && arrayElementIdx == -1 )
        {
            pRow = New<ArrayRow>( pParentRow, context, propertyInfo, pTypeInstance );
        }
        else
        {
            pRow = New<PropertyRow>( pParentRow, context, propertyInfo, pTypeInstance, arrayElementIdx );
        }

        return pRow;
    }

    //-------------------------------------------------------------------------

    void GridRow::UpdateRow()
    {
        // Children are updated before parents so parents can inspect child state
        for ( auto& child : m_children )
        {
            child->Update();
        }

        // Perform Update
        Update();
    }

    void GridRow::DrawRow( float currentHeaderOffset )
    {
        //-------------------------------------------------------------------------
        // Draw
        //-------------------------------------------------------------------------

        bool const shouldDrawRow = ShouldDrawRow();
        if ( shouldDrawRow )
        {
            ImGui::PushID( this );
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                DrawHeaderSection( currentHeaderOffset );

                ImGui::TableNextColumn();
                {
                    ImGuiTableFlags const flags = ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingFixedFit;
                    ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 0 ) );
                    if ( ImGui::BeginTable( "GridTable", HasExtraControls() ? 3 : 2, flags ) )
                    {
                        ImGui::TableSetupColumn( "##Editor", ImGuiTableColumnFlags_WidthStretch );

                        if ( HasExtraControls() )
                        {
                            ImGui::TableSetupColumn( "##Extra", ImGuiTableColumnFlags_WidthFixed, GetExtraControlsSectionWidth() );
                        }

                        ImGui::TableSetupColumn( "##Reset", ImGuiTableColumnFlags_WidthFixed, g_controlButtonSize.x );

                        //-------------------------------------------------------------------------

                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();
                        DrawEditorSection();

                        if ( HasExtraControls() )
                        {
                            ImGui::TableNextColumn();
                            DrawExtraControlsSection();
                        }

                        ImGui::TableNextColumn();
                        if ( HasResetSection() )
                        {
                            DrawResetSection();
                        }

                        ImGui::EndTable();
                    }
                    ImGui::PopStyleVar();
                }
            }
            ImGui::PopID();
        }

        //-------------------------------------------------------------------------
        // Draw Children
        //-------------------------------------------------------------------------

        if ( m_isExpanded )
        {
            ImGui::BeginDisabled( IsReadOnly() );

            float const childHeaderOffset = currentHeaderOffset + g_headerOffset;
            for ( auto& child : m_children )
            {
                child->DrawRow( childHeaderOffset );
            }

            ImGui::EndDisabled();
        }
    }

    void GridRow::SetExpansion( bool isExpanded )
    {
        m_isExpanded = isExpanded;

        for ( auto pChild : m_children )
        {
            pChild->SetExpansion( isExpanded );
        }
    }

    void GridRow::DestroyChildren()
    {
        for ( auto pChild : m_children )
        {
            pChild->DestroyChildren();
            Delete( pChild );
        }

        m_children.Clear();
    }

    //-------------------------------------------------------------------------

    CategoryRow* CategoryRow::FindOrCreateCategory( GridRow* pParentRow, GridContext const& context, List<CategoryRow*>& categories, String const& categoryName )
    {
        int32 insertIdx = 0;
        int32 foundCategoryIdx = -1;
        for ( int32 i = 0; i < categories.Count(); i++ )
        {
            if ( categories[i]->GetName() < categoryName )
            {
                insertIdx++;
            }

            if ( categories[i]->GetName() == categoryName )
            {
                foundCategoryIdx = i;
                break;
            }
        }

        if ( foundCategoryIdx != -1 )
        {
            return categories[foundCategoryIdx];
        }
        else
        {
            auto pNewCategory = New<CategoryRow>( pParentRow, context, categoryName );
            categories.Insert(insertIdx, pNewCategory );
            return pNewCategory;
        }
    }

    CategoryRow::CategoryRow( GridRow* pParentRow, GridContext const& context, String const& name )
        : GridRow( pParentRow, context, name )
    {}

    void CategoryRow::DrawHeaderSection( float currentHeaderOffset )
    {
        ImGui::Dummy( ImVec2( currentHeaderOffset, 0 ) );
        ImGui::SameLine();

        ImGui::SetNextItemOpen( m_isExpanded );
        ImGui::PushStyleColor( ImGuiCol_Header, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderActive, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, 0 );
        m_isExpanded = ImGui::CollapsingHeader(m_name.ToStringAnsi().Get());
        ImGui::PopStyleColor( 3 );

        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, Color32(GUI::Style::s_colorGray7));
    }

    void CategoryRow::AddProperty( IType* pTypeInstance, TypeProperty const& propertyInfo )
    {
        ENGINE_ASSERT( pTypeInstance != nullptr );

        GridRow* pRow = CreateRow( this, m_context, pTypeInstance, propertyInfo );

        // Sorted Insert
        //-------------------------------------------------------------------------

        size_t insertIdx = 0;
        for ( ; insertIdx < m_children.Count(); insertIdx++ )
        {
            if ( m_children[insertIdx]->GetName() > pRow->GetName() )
            {
                break;
            }
        }

        m_children.Insert(insertIdx, pRow );
    }

    bool CategoryRow::ShouldDrawRow() const
    {
        // Dont show empty categories
        if ( m_children.IsEmpty() )
        {
            return false;
        }

        // Always show categories if we have a visible child
        for ( auto pChildRow : m_children )
        {
            if ( pChildRow->ShouldDrawRow() )
            {
                return true;
            }
        }

        // The 'm_isHidden' flag doesnt apply to category rows, we only care about the children
        return false;
    }

    //-------------------------------------------------------------------------

    ArrayRow::ArrayRow( GridRow* pParentRow, GridContext const& context, TypeProperty const& propertyInfo, IType* pParentTypeInstance )
        : GridRow( pParentRow, context, propertyInfo.name, propertyInfo.isToolsReadOnly )
        , m_pParentTypeInstance( pParentTypeInstance )
        , m_propertyInfo( propertyInfo )
    {
        ENGINE_ASSERT( m_propertyInfo.IsArrayProperty() );
        RebuildChildren();
    }

    void ArrayRow::MoveElementUp( int32 arrayElementIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );

        ENGINE_ASSERT( m_operationElementIdx == -1 );
        ENGINE_ASSERT( arrayElementIndex > 0 && arrayElementIndex < arraySize );
        m_operationType = OperationType::MoveUp;
        m_operationElementIdx = arrayElementIndex;
    }

    void ArrayRow::MoveElementDown( int32 arrayElementIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );

        ENGINE_ASSERT( m_operationElementIdx == -1 );
        ENGINE_ASSERT( arrayElementIndex >= 0 && arrayElementIndex < ( arraySize - 1 ) );
        m_operationType = OperationType::MoveDown;
        m_operationElementIdx = arrayElementIndex;
    }

    void ArrayRow::InsertElement( int32 insertIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );

        ENGINE_ASSERT( m_operationElementIdx == -1 );
        ENGINE_ASSERT( insertIndex >= 0 && insertIndex < arraySize );
        m_operationType = OperationType::Insert;
        m_operationElementIdx = insertIndex;
    }

    void ArrayRow::DestroyElement( int32 arrayElementIndex )
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );

        ENGINE_ASSERT( m_operationElementIdx == -1 );
        ENGINE_ASSERT( arrayElementIndex >= 0 && arrayElementIndex < arraySize );
        m_operationType = OperationType::Remove;
        m_operationElementIdx = arrayElementIndex;
    }

    void ArrayRow::Update()
    {
        // External Update Check
        //-------------------------------------------------------------------------

        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );
        if ( m_children.Count() != arraySize )
        {
            RebuildChildren();
        }

        // Element Operations
        //-------------------------------------------------------------------------

        switch ( m_operationType )
        {
        case OperationType::Insert:
        {
            ENGINE_ASSERT( m_operationElementIdx >= 0 && m_operationElementIdx < m_children.Count() );
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::AddArrayElement );
            m_pParentTypeInstance->GetTypeInfo()->InsertArrayElement( m_pParentTypeInstance, m_propertyInfo.id, m_operationElementIdx );
            RebuildChildren();
        }
            break;

        case OperationType::MoveUp:
        {
            ENGINE_ASSERT( m_operationElementIdx > 0 && m_operationElementIdx < m_children.Count() );
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::MoveArrayElement );
            m_pParentTypeInstance->GetTypeInfo()->MoveArrayElement( m_pParentTypeInstance, m_propertyInfo.id, m_operationElementIdx, m_operationElementIdx - 1 );
            RebuildChildren();
        }
            break;

        case OperationType::MoveDown:
        {
            ENGINE_ASSERT( m_operationElementIdx >= 0 && m_operationElementIdx < ( m_children.Count() - 1 ) );
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::MoveArrayElement );
            m_pParentTypeInstance->GetTypeInfo()->MoveArrayElement( m_pParentTypeInstance, m_propertyInfo.id, m_operationElementIdx, m_operationElementIdx + 1 );
            RebuildChildren();
        }
            break;

        case OperationType::Remove:
        {
            ENGINE_ASSERT( m_operationElementIdx >= 0 && m_operationElementIdx < m_children.Count() );
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::RemoveArrayElement );
            m_pParentTypeInstance->GetTypeInfo()->RemoveArrayElement( m_pParentTypeInstance, m_propertyInfo.id, m_operationElementIdx );
            RebuildChildren();
        }
            break;

        default:
            break;
        }

        //-------------------------------------------------------------------------

        // No operation last more than a frame
        m_operationType = OperationType::None;
        m_operationElementIdx = -1;

        // Visibility
        //-------------------------------------------------------------------------
        // Array element visibility is tied to our visibility!

        if ( !m_isHidden )
        {
            for ( auto pChild : m_children )
            {
                pChild->SetHidden( false );
            }
        }
    }

    bool ArrayRow::ShouldDrawRow() const
    {
        for ( auto pChildRow : m_children )
        {
            if ( pChildRow->ShouldDrawRow() )
            {
                return true;
            }
        }

        return !m_isHidden;
    }

    void ArrayRow::DrawHeaderSection( float currentHeaderOffset )
    {
        ImGui::Dummy( ImVec2( currentHeaderOffset, 0 ) );
        ImGui::SameLine();

        ImGui::SetNextItemOpen( m_isExpanded );
        ImGui::PushStyleColor( ImGuiCol_Header, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderActive, 0 );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, 0 );
        m_isExpanded = ImGui::CollapsingHeader( m_name.ToStringAnsi().Get() );
        ImGui::PopStyleColor( 3 );

        if ( !m_propertyInfo.description.IsEmpty() )
        {
            ImGui::ItemTooltip( m_propertyInfo.description.Get() );
        }
    }

    void ArrayRow::DrawEditorSection()
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );
        ImGui::TextColored( Colors::Gray.ToFloat4(), "%d Elements", arraySize );
    }

    bool ArrayRow::HasExtraControls() const
    {
        return m_propertyInfo.IsDynamicArrayProperty() && !m_propertyInfo.showInRestrictedMode;
    }

    float ArrayRow::GetExtraControlsSectionWidth() const
    {
        return ( g_controlButtonSize.x * 2 ) + 4;
    }

    void ArrayRow::DrawExtraControlsSection()
    {
        ImGui::BeginDisabled( IsReadOnly() );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGui::FlatButtonColored( Colors::LightGreen, ICON_PLUS, g_controlButtonSize ) )
        {
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::AddArrayElement );
            m_pParentTypeInstance->GetTypeInfo()->AddArrayElement( m_pParentTypeInstance, m_propertyInfo.id );
            RebuildChildren();
            m_isExpanded = true;

        }
        ImGui::PopStyleVar();
        ImGui::ItemTooltip(SE_TEXT("Add array element"));

        ImGui::SameLine();
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGui::FlatButtonColored( Colors::PaleVioletRed, ICON_TRASH_CAN, g_controlButtonSize ) )
        {
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo, PropertyEditInfo::Action::RemoveArrayElement );
            m_pParentTypeInstance->GetTypeInfo()->ClearArray( m_pParentTypeInstance, m_propertyInfo.id );
            RebuildChildren();
        }
        ImGui::PopStyleVar();
        ImGui::ItemTooltip(SE_TEXT("Remove all array elements"));

        ImGui::EndDisabled();
    }

    bool ArrayRow::HasResetSection() const
    {
        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        return !pTypeInfo->IsPropertyValueSetToDefault( m_pParentTypeInstance, m_propertyInfo.id );
    }

    void ArrayRow::DrawResetSection()
    {
        ImGui::BeginDisabled( IsReadOnly() );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGui::FlatButtonColored( Colors::LightGray, ICON_UNDO_VARIANT, g_controlButtonSize ) )
        {
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo );
            m_pParentTypeInstance->GetTypeInfo()->ResetToDefault( m_pParentTypeInstance, m_propertyInfo.id );
            RebuildChildren();
        }
        ImGui::PopStyleVar();
        ImGui::ItemTooltip(SE_TEXT("Reset value to default"));

        ImGui::EndDisabled();
    }

    void ArrayRow::RebuildChildren()
    {
        DestroyChildren();

        //-------------------------------------------------------------------------

        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );
        for ( auto i = 0u; i < arraySize; i++ )
        {
            m_children.Add( CreateRow( this, m_context, m_pParentTypeInstance, m_propertyInfo, i ) );
        }
    }

    //-------------------------------------------------------------------------

    PropertyRow::PropertyRow( GridRow* pParentRow, GridContext const& context, TypeProperty const& propertyInfo, IType* pParentTypeInstance, int32 arrayElementIndex )
        : GridRow( pParentRow, context, propertyInfo.name, propertyInfo.isToolsReadOnly )
        , m_propertyInfo( propertyInfo )
        , m_pParentTypeInstance( pParentTypeInstance )
        , m_arrayElementIdx( arrayElementIndex )
    {
        ENGINE_ASSERT( m_pParentTypeInstance != nullptr );
        // ENGINE_ASSERT( propertyInfo. != TypeID::Invalid );

        //-------------------------------------------------------------------------

        if ( arrayElementIndex != -1 )
        {
            m_name = StringUtils::ToString(m_arrayElementIdx);
            m_pPropertyInstance = m_pParentTypeInstance->GetTypeInfo()->GetArrayElementDataPtr( m_pParentTypeInstance, propertyInfo.id, m_arrayElementIdx );
        }
        else
        {
            m_pPropertyInstance = reinterpret_cast<uint8_t*>( m_pParentTypeInstance ) + propertyInfo.offset;
        }

        //-------------------------------------------------------------------------

        m_pPropertyEditor = PropertyGridEditorFactory::TryCreateEditor( m_context, m_propertyInfo, m_pPropertyInstance );
        m_pTypeEditingRules = TypeEditingRulesFactory::TryCreateRules( m_context.m_pEditorContext, m_pParentTypeInstance );

        RebuildChildren();
    }

    PropertyRow::~PropertyRow()
    {
        Delete( m_pPropertyEditor );
        Delete( m_pTypeEditingRules );
    }

    bool PropertyRow::ShouldDrawRow() const
    {
        if ( m_propertyInfo.IsStructureProperty() && !HasPropertyEditor() )
        {
            for ( auto pChildRow : m_children )
            {
                if ( pChildRow->ShouldDrawRow() )
                {
                    return true;
                }
            }

            return false;
        }
        else
        {
            return !m_isHidden;
        }
    }

    void PropertyRow::Update()
    {
        if ( m_pTypeEditingRules != nullptr )
        {
            if ( !m_isDeclaredReadOnly )
            {
                m_isReadOnly = m_pTypeEditingRules->IsReadOnly( m_propertyInfo.id );
            }

            m_isHidden = m_pTypeEditingRules->IsHidden( m_propertyInfo.id );
        }
    }

    void PropertyRow::DrawHeaderSection( float currentHeaderOffset )
    {
        ImGui::Dummy( ImVec2( currentHeaderOffset, 0 ) );
        ImGui::SameLine();

        //-------------------------------------------------------------------------

        if ( HasPropertyEditor() )
        {
            if ( !m_propertyInfo.description.IsEmpty() )
            {
                ImGui::Label(m_name.Get(), m_propertyInfo.description.Get());
            }
            else
            {
                ImGui::Label(m_name.Get());
            }
        }
        else if ( m_propertyInfo.IsStructureProperty() )
        {
            ImGui::SetNextItemOpen( m_isExpanded );
            ImGui::PushStyleColor( ImGuiCol_Header, 0 );
            ImGui::PushStyleColor( ImGuiCol_HeaderActive, 0 );
            ImGui::PushStyleColor( ImGuiCol_HeaderHovered, 0 );
            m_isExpanded = ImGui::CollapsingHeader( m_name.ToStringAnsi().Get() );
            ImGui::PopStyleColor( 3 );

            if ( !m_propertyInfo.description.IsEmpty() )
            {
                ImGui::ItemTooltip( m_propertyInfo.description.Get() );
            }
        }
        else
        {
            ImGui::Label(m_name);
        }
    }

    void PropertyRow::DrawEditorSection()
    {
        if ( HasPropertyEditor() )
        {
            ImGui::BeginDisabled( IsReadOnly() );
            if ( m_pPropertyEditor->UpdateAndDraw() )
            {
                ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo );
                m_pPropertyEditor->UpdatePropertyValue();
            }
            ImGui::EndDisabled();
        }
        else if ( m_propertyInfo.IsStructureProperty() )
        {
            auto pTypeInfo = reinterpret_cast<IType*>( m_pPropertyInstance )->GetTypeInfo();
            String const structTypeName = String::Format(SE_TEXT("Type: [{}]"), pTypeInfo->id);
            ImGui::TextColored( Colors::Gray.ToFloat4(), structTypeName.ToStringAnsi().Get() );
        }
        else
        {
            ImGui::Text(ICON_EXCLAMATION" Missing Editor" );
        }
    }

    bool PropertyRow::HasExtraControls() const
    {
        if ( m_propertyInfo.IsStaticArrayProperty() )
        {
            return false;
        }

        bool const isDynamicArrayElement = ( m_arrayElementIdx != -1 );
        if ( isDynamicArrayElement )
        {
            return !m_propertyInfo.showInRestrictedMode;
        }

        return false;
    }

    float PropertyRow::GetExtraControlsSectionWidth() const
    {
        return g_controlButtonSize.x;
    }

    void PropertyRow::DrawExtraControlsSection()
    {
        // Draw array element options
        //-------------------------------------------------------------------------

        ENGINE_ASSERT( m_arrayElementIdx >= 0 );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        ImGui::FlatButtonColored( GUI::Style::s_colorText, ICON_DOTS_HORIZONTAL, g_controlButtonSize );
        ImGui::PopStyleVar();

        if ( ImGui::BeginPopupContextItem( 0, ImGuiPopupFlags_MouseButtonLeft ) )
        {
            if ( ImGui::MenuItem( SE_TEXT(ICON_PLUS" Insert New Element")))
            {
                static_cast<ArrayRow*>( m_pParent )->InsertElement( m_arrayElementIdx );
            }

            if ( m_arrayElementIdx > 0 )
            {
                if ( ImGui::MenuItem( SE_TEXT(ICON_ARROW_UP" Move Element Up")))
                {
                    static_cast<ArrayRow*>( m_pParent )->MoveElementUp( m_arrayElementIdx );
                }
            }

            auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
            size_t const arraySize = pTypeInfo->GetArraySize( m_pParentTypeInstance, m_propertyInfo.id );
            if ( m_arrayElementIdx < ( arraySize - 1 ) )
            {
                if ( ImGui::MenuItem( SE_TEXT(ICON_ARROW_DOWN" Move Element Down")))
                {
                    static_cast<ArrayRow*>( m_pParent )->MoveElementDown( m_arrayElementIdx );
                }
            }

            if ( ImGui::MenuItem( SE_TEXT(ICON_TRASH_CAN" Remove Element")))
            {
                static_cast<ArrayRow*>( m_pParent )->DestroyElement( m_arrayElementIdx );
            }
            ImGui::EndPopup();
        }

        ImGui::ItemTooltip(SE_TEXT("Array Element Options"));
    }

    bool PropertyRow::HasResetSection() const
    {
        if ( m_arrayElementIdx != -1 )
        {
            return false;
        }

        auto pTypeInfo = m_pParentTypeInstance->GetTypeInfo();
        return !pTypeInfo->IsPropertyValueSetToDefault( m_pParentTypeInstance, m_propertyInfo.id, m_arrayElementIdx );
    }

    void PropertyRow::DrawResetSection()
    {
        ImGui::BeginDisabled( IsReadOnly() );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, g_controlButtonPadding );
        if ( ImGui::FlatButtonColored( Colors::LightGray, ICON_UNDO_VARIANT, g_controlButtonSize ) )
        {
            ScopedChangeNotifier cn( m_context.m_pPropertyGrid, m_pParentTypeInstance, &m_propertyInfo );
            m_pParentTypeInstance->GetTypeInfo()->ResetToDefault( m_pParentTypeInstance, m_propertyInfo.id );

            if ( HasPropertyEditor() )
            {
                m_pPropertyEditor->ResetWorkingCopy();
            }
            else
            {
                RebuildChildren();
            }
        }
        ImGui::PopStyleVar();
        ImGui::ItemTooltip(SE_TEXT("Reset value to default" ));
        ImGui::EndDisabled();
    }

    void PropertyRow::RebuildChildren()
    {
        DestroyChildren();

        if ( !m_propertyInfo.IsStructureProperty() || HasPropertyEditor() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        List<CategoryRow*> categories(10);

        auto pStructTypeInstance = reinterpret_cast<IType*>( m_pPropertyInstance );
        TypeCompositeInfo const* pTypeInfo = pStructTypeInstance->GetTypeInfo();
        for ( auto const& propertyInfo : pTypeInfo->properties )
        {
            PG::CategoryRow* pCategory = PG::CategoryRow::FindOrCreateCategory( this, m_context, categories,
                propertyInfo->category.IsEmpty() ? SE_TEXT("General") : propertyInfo->category );
            pCategory->AddProperty( pStructTypeInstance, *propertyInfo );
        }

        //-------------------------------------------------------------------------

        // If we only have a single category, there's no point polluting the grid with it
        if ( categories.Count() == 1 )
        {
            for ( auto pProperty : categories[0]->GetChildren() )
            {
                pProperty->SetParent( this );
                m_children.Add( pProperty );
            }

            Delete( categories[0] );
        }
        else // Add all categories
        {
            for ( auto pCategory : categories )
            {
                m_children.Add( pCategory );
            }
        }
    }
}
