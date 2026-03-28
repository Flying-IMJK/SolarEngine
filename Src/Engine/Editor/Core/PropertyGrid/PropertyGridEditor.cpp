#include "PropertyGridEditor.h"
#include "Editor/EditorContext.h"
// #include "Editor/Core/Widgets/CurveEditor.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/Info/TypeEnumInfo.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Strings/StringConverter.h"

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Property Grid Editor Base
//-------------------------------------------------------------------------

namespace SE::Editor::PG
{
    PropertyEditor::PropertyEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
        : m_context( context )
        , m_propertyInfo( propertyInfo )
        , m_pPropertyInstance( m_pPropertyInstance )
        , m_coreType( GetCoreType( propertyInfo.typeID ) )
    {
        ENGINE_ASSERT( m_pPropertyInstance != nullptr );
    }

    bool PropertyEditor::UpdateAndDraw()
    {
        ImGui::PushID( m_pPropertyInstance );
        HandleExternalUpdate();
        bool const result = InternalUpdateAndDraw();
        ImGui::PopID();

        return result;
    }
}

//-------------------------------------------------------------------------
// Core Property Grid Editors
//-------------------------------------------------------------------------

namespace SE::Editor::PG
{
    constexpr static float const g_iconButtonWidth = 30;

    //-------------------------------------------------------------------------
    // Core Editors
    //-------------------------------------------------------------------------

    class EnumEditor final : public PropertyEditor
    {
    public:

        EnumEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
            , m_pEnumInfo( Types::GetEnumInfo( m_propertyInfo.typeID ) )
        {
            ENGINE_ASSERT( m_pEnumInfo != nullptr );
            EnumEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            if ( ImGui::BeginCombo(SE_TEXT("##enumCombo"), m_pEnumInfo->GetConstantLabel( m_value_imgui ).ToString() ) )
            {
                for ( auto const& enumValue : m_pEnumInfo->constants )
                {
                    bool const isSelected = ( enumValue.value == m_value_imgui );
                    if ( ImGui::Selectable( enumValue.id.ToString(), isSelected ) )
                    {
                        m_value_imgui = enumValue.value;
                    }

                    if ( !enumValue.description.IsEmpty() )
                    {
                        ImGui::ItemTooltip( enumValue.description.Get());
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if ( isSelected )
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

				ImGui::EndCombo();
            }

            return m_value_cached != m_value_imgui;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;

            switch ( m_pEnumInfo->underlyingType )
            {
                case TypeIDCore::Uint8:
                {
                    *reinterpret_cast<uint8*>( m_pPropertyInstance ) = (uint8)m_value_cached;
                }
                break;

                case TypeIDCore::Int8:
                {
                    *reinterpret_cast<int8*>( m_pPropertyInstance ) = (int8)m_value_cached;
                }
                break;

                case TypeIDCore::Uint16:
                {
                    *reinterpret_cast<uint16*>( m_pPropertyInstance ) = (uint16)m_value_cached;
                }
                break;

                case TypeIDCore::Int16:
                {
                    *reinterpret_cast<int16*>( m_pPropertyInstance ) = (int16)m_value_cached;
                }
                break;

                case TypeIDCore::Uint32:
                {
                    *reinterpret_cast<uint32*>( m_pPropertyInstance ) = (uint32)m_value_cached;
                }
                break;

                case TypeIDCore::Int32:
                {
                    *reinterpret_cast<int32*>( m_pPropertyInstance ) = (int32)m_value_cached;
                }
                break;

                default:
                {
                    ENGINE_UNREACHABLE_CODE();
                }
                break;
            }
        }

        virtual void ResetWorkingCopy() override
        {
            switch ( m_pEnumInfo->underlyingType )
            {
                case TypeIDCore::Uint8:
                {
                    m_value_cached = m_value_imgui = ( int64 ) * reinterpret_cast<uint8 const*>( m_pPropertyInstance );
                }
                break;

                case TypeIDCore::Int8:
                {
                    m_value_cached = m_value_imgui = ( int64) * reinterpret_cast<int8 const*>( m_pPropertyInstance );
                }
                break;

                case TypeIDCore::Uint16:
                {
                    m_value_cached = m_value_imgui = ( int64 ) * reinterpret_cast<uint16 const*>( m_pPropertyInstance );
                }
                break;

                case TypeIDCore::Int16:
                {
                    m_value_cached = m_value_imgui = ( int64 ) * reinterpret_cast<int16 const*>( m_pPropertyInstance );
                }
                break;

                case TypeIDCore::Uint32:
                {
                    m_value_cached = m_value_imgui = ( int64 ) * reinterpret_cast<uint32 const*>( m_pPropertyInstance );
                }
                break;

                case TypeIDCore::Int32:
                {
                    m_value_cached = m_value_imgui = ( int64 ) * reinterpret_cast<int32 const*>( m_pPropertyInstance );
                }
                break;

                default:
                {
                    ENGINE_UNREACHABLE_CODE();
                }
                break;
            }
        }

        virtual void HandleExternalUpdate() override
        {
            switch ( m_pEnumInfo->underlyingType )
            {
                case TypeIDCore::Uint8:
                {
                    auto actualValue = ( int64 ) * reinterpret_cast<uint8 const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case TypeIDCore::Int8:
                {
                    auto actualValue = ( int64 ) * reinterpret_cast<int8 const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case TypeIDCore::Uint16:
                {
                    auto actualValue = ( int64 ) * reinterpret_cast<uint16 const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case TypeIDCore::Int16:
                {
                    auto actualValue = ( int64 ) * reinterpret_cast<int16 const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case TypeIDCore::Uint32:
                {
                    auto actualValue = ( int64 ) * reinterpret_cast<uint32 const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case TypeIDCore::Int32:
                {
                    auto actualValue = ( int64 ) * reinterpret_cast<int32 const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                default:
                {
                    ENGINE_UNREACHABLE_CODE();
                }
                break;
            }
        }

    private:

        TypeEnumInfo const*     m_pEnumInfo = nullptr;
        int64               m_value_cached;
        int64               m_value_imgui;
    };

    //-------------------------------------------------------------------------

    class BoolEditor final : public PropertyEditor
    {
    public:

        BoolEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            BoolEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::Checkbox( "##be", &m_value );
            return ImGui::IsItemDeactivatedAfterEdit();
        }

        virtual void UpdatePropertyValue() override
        {
            *reinterpret_cast<bool*>( m_pPropertyInstance ) = m_value;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value = *reinterpret_cast<bool*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            ResetWorkingCopy();
        }

    private:

        bool m_value;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class ScalarEditor final : public PropertyEditor
    {
    public:

        ScalarEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            ScalarEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );

            switch ( m_coreType )
            {
                case TypeIDCore::Int8:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S8, &m_value_imgui );
                }
                break;

                case TypeIDCore::Int16:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S16, &m_value_imgui );
                }
                break;

                case TypeIDCore::Int32:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S32, &m_value_imgui );
                }
                break;

                case TypeIDCore::Int64:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S64, &m_value_imgui );
                }
                break;

                case TypeIDCore::Uint8:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U8, &m_value_imgui );
                }
                break;

                case TypeIDCore::Uint16:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U16, &m_value_imgui );
                }
                break;

                case TypeIDCore::Uint32:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U32, &m_value_imgui );
                }
                break;

                case TypeIDCore::Uint64:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U64, &m_value_imgui );
                }
                break;

                case TypeIDCore::Float:
                {
                    ImGui::InputFloat( "##scaed", (float*)&m_value_imgui );
                }
                break;

                case TypeIDCore::Double:
                {
                    ImGui::InputDouble( "##scaed", (double*)&m_value_imgui );
                }
                break;

                default:
                {
                    ENGINE_UNREACHABLE_CODE();
                }
                break;
            }

            return ImGui::IsItemDeactivatedAfterEdit();
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<T*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<T*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            T const actualValue = *reinterpret_cast<T*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        T                   m_value_imgui;
        T                   m_value_cached;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class FloatNEditor final : public PropertyEditor
    {
    public:

        FloatNEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            FloatNEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            switch ( m_coreType )
            {
                case TypeIDCore::Float2:
                {
                    if ( ImGui::InputFloat2( "F2Ed", (Float2&)m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                case TypeIDCore::Float3:
                {
                    if ( ImGui::InputFloat3( "F3Ed", (Float3&)m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                case TypeIDCore::Float4:
                {
                    if ( ImGui::InputFloat4( "F4Ed", (Float4&)m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                default:
                {
                    ENGINE_UNREACHABLE_CODE();
                }
                break;
            }

            return valueChanged;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<T*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<T*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            T const actualValue = *reinterpret_cast<T*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        T                   m_value_imgui;
        T                   m_value_cached;
    };

    //-------------------------------------------------------------------------

    class ColorEditor final : public PropertyEditor
    {
    public:

        ColorEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            ColorEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            ImGui::ColorEdit4( "##ce", &m_value_imgui.x );
            return ImGui::IsItemDeactivatedAfterEdit();
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Color32*>( m_pPropertyInstance ) = Color32( m_value_cached );
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = reinterpret_cast<Color32*>( m_pPropertyInstance )->ToFloat4();
        }

        virtual void HandleExternalUpdate() override
        {
            Float4 const actualValue = reinterpret_cast<Color32*>( m_pPropertyInstance )->ToFloat4();
            if ( !Float4::NearEqual(actualValue, Float4(m_value_cached) ) )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        ImVec4 m_value_imgui;
        ImVec4 m_value_cached;
    };

    //-------------------------------------------------------------------------

    class UUIDEditor final : public PropertyEditor
    {
    public:

        UUIDEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            UUIDEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            float const textAreaWidth = ImGui::GetContentRegionAvail().x - g_iconButtonWidth - ImGui::GetStyle().ItemSpacing.x;

            bool valueChanged = false;

            ImGui::SetNextItemWidth( textAreaWidth );
            ImGui::InputText( "##ue", m_stringValue, ImGuiInputTextFlags_ReadOnly );

            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            if ( ImGui::Button(ICON_COG"##Generate", ImVec2( g_iconButtonWidth, 0 ) ) )
            {
				Platform::CreateUUID(m_value_imgui);
                m_stringValue = m_value_imgui.ToString();
                valueChanged = true;
            }
            ImGui::ItemTooltip(SE_TEXT("Generate UUID"));

            return valueChanged;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<UID*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<UID*>( m_pPropertyInstance );
            m_stringValue = m_value_imgui.ToString();
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<UID*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
                m_stringValue = m_value_imgui.ToString();
            }
        }

    private:

        UID           m_value_imgui;
		UID           m_value_cached;
        String          m_stringValue;
    };

    //-------------------------------------------------------------------------

/*    class TimeEditor final : public PropertyEditor
    {
    public:

        TimeEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            TimeEditor::ResetWorkingCopy();
            ENGINE_ASSERT( m_coreType == TypeIDCore::Microseconds || m_coreType == TypeIDCore::Milliseconds || m_coreType == TypeIDCore::Seconds );
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            if ( m_coreType == TypeIDCore::Microseconds )
            {
                ImGui::InputFloat( "##teus", &m_value_imgui, 0.0f, 0.0f, "%.2fus" );
            }
            else if ( m_coreType == TypeIDCore::Milliseconds )
            {
                ImGui::InputFloat( "#tems", &m_value_imgui, 0.0f, 0.0f, "%.2fms" );
            }
            else if ( m_coreType == TypeIDCore::Seconds )
            {
                ImGui::InputFloat( "##tes", &m_value_imgui, 0.0f, 0.0f, "%.2fs" );
            }

            return ImGui::IsItemDeactivatedAfterEdit();
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;

            if ( m_coreType == TypeIDCore::Microseconds )
            {
                *reinterpret_cast<Microseconds*>( m_pPropertyInstance ) = Microseconds( m_value_cached );
            }
            else if ( m_coreType == TypeIDCore::Milliseconds )
            {
                *reinterpret_cast<Milliseconds*>( m_pPropertyInstance ) = Milliseconds( m_value_cached );
            }
            else if ( m_coreType == TypeIDCore::Seconds )
            {
                *reinterpret_cast<Seconds*>( m_pPropertyInstance ) = Seconds( m_value_cached );
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == TypeIDCore::Microseconds )
            {
                m_value_cached = m_value_imgui = reinterpret_cast<Microseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == TypeIDCore::Milliseconds )
            {
                m_value_cached = m_value_imgui = reinterpret_cast<Milliseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == TypeIDCore::Seconds )
            {
                m_value_cached = m_value_imgui = reinterpret_cast<Seconds*>( m_pPropertyInstance )->ToFloat();
            }
        }

        virtual void HandleExternalUpdate() override
        {
            float actualValue = 0;

            if ( m_coreType == TypeIDCore::Microseconds )
            {
                actualValue = reinterpret_cast<Microseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == TypeIDCore::Milliseconds )
            {
                actualValue = reinterpret_cast<Milliseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == TypeIDCore::Seconds )
            {
                actualValue = reinterpret_cast<Seconds*>( m_pPropertyInstance )->ToFloat();
            }

            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        float                m_value_imgui;
        float                m_value_cached;
    };*/

    //-------------------------------------------------------------------------
    /*
    class PercentageEditor final : public PropertyEditor
    {
    public:

        PercentageEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            PercentageEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            float const textAreaWidth = ImGui::GetContentRegionAvail().x - g_iconButtonWidth - ImGui::GetStyle().ItemSpacing.x;

            ImGui::SetNextItemWidth( textAreaWidth );
            ImGui::InputFloat( "##pe", &m_value_imgui, 0, 0, "%.2f%%" );
            bool valueChanged = ImGui::IsItemDeactivatedAfterEdit();

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            if ( ImGui::Button(ICON_PERCENT_BOX_OUTLINE"##ClampPercentage", ImVec2( g_iconButtonWidth, 0 ) ) )
            {
                m_value_imgui = ( Percentage( m_value_imgui / 100 ).GetClamped( true ) ).ToFloat() * 100;
                valueChanged = true;
            }
            SGUI::ItemTooltip( "Clamp [-100 : 100]" );

            return valueChanged;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Percentage*>( m_pPropertyInstance ) = Percentage( m_value_cached / 100.0f );
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = reinterpret_cast<Percentage*>( m_pPropertyInstance )->ToFloat() * 100;
        }

        virtual void HandleExternalUpdate() override
        {
            auto actualValue = reinterpret_cast<Percentage*>( m_pPropertyInstance )->ToFloat() * 100;;
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        float  m_value_imgui;
        float  m_value_cached;
    };
    */
    //-------------------------------------------------------------------------

    /*class TransformEditor final : public PropertyEditor
    {
    public:

        TransformEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            TransformEditor::ResetWorkingCopy();
            ENGINE_ASSERT( m_coreType == TypeIDCore::Transform || m_coreType == TypeIDCore::Matrix );
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool transformUpdated = false;
            constexpr float const headerWidth = 70;

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
            if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None ) )
            {
                ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, headerWidth );
                ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide );

                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    {
                        GUI::ScopedFont const sf( GUI::Font::Small );
                        ImGui::Text( "Translation" );
                    }

                    ImGui::TableNextColumn();
                    if ( SGUI::InputFloat3( "T", m_translation_imgui ) )
                    {
                        m_translation_cached = m_translation_imgui;
                        transformUpdated = true;
                    }
                }

                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    {
                        GUI::ScopedFont const sf( GUI::Font::Small );
                        ImGui::Text( "Rotation" );
                    }

                    ImGui::TableNextColumn();
                    if ( SGUI::InputFloat3( "R", m_rotation_imgui ) )
                    {
                        m_rotation_imgui.x = Degrees( m_rotation_imgui.x ).GetClamped180().ToFloat();
                        m_rotation_imgui.y = Degrees( m_rotation_imgui.y ).GetClamped180().ToFloat();
                        m_rotation_imgui.z = Degrees( m_rotation_imgui.z ).GetClamped180().ToFloat();
                        m_rotation_cached = m_rotation_imgui;
                        transformUpdated = true;
                    }
                }

                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();

                    GUI::ScopedFont const sf( GUI::Font::Small );
                    ImGui::Text( "Scale" );

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth( -1 );
                    if ( ImGui::InputFloat( "##S", &m_scale_imgui ) )
                    {
                        m_scale_imgui = Math::IsNearZero( m_scale_imgui ) ? 0.01f : m_scale_imgui;
                        m_scale_cached = m_scale_imgui;
                        transformUpdated = true;
                    }
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();

            return transformUpdated;
        }

        virtual void UpdatePropertyValue() override
        {
            EulerAngles const rot( m_rotation_cached );
            Quaternion const q( rot );

            if ( m_coreType == TypeIDCore::Transform )
            {
                *reinterpret_cast<Transform*>( m_pPropertyInstance ) = Transform( q, m_translation_cached, m_scale_cached );
            }
            else if ( m_coreType == TypeIDCore::Matrix )
            {
                *reinterpret_cast<Matrix*>( m_pPropertyInstance ) = Matrix( q, m_translation_cached, m_scale_cached );
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == TypeIDCore::Transform )
            {
                auto const& transform = reinterpret_cast<Transform*>( m_pPropertyInstance );

                m_rotation_cached = m_rotation_imgui = transform->GetRotation().ToEulerAngles().GetAsDegrees();
                m_translation_cached = m_translation_imgui = transform->GetTranslation().ToFloat3();
                m_scale_cached = m_scale_imgui = transform->GetScale();
            }
            else if ( m_coreType == TypeIDCore::Matrix )
            {
                auto const& matrix = reinterpret_cast<Matrix*>( m_pPropertyInstance );

                Quaternion q;
                VectorSIMD t;
                VectorSIMD s;
                matrix->Decompose( q, t, s );

                m_rotation_cached = m_rotation_imgui = q.ToEulerAngles().GetAsDegrees();
                m_translation_cached = m_translation_imgui = t.ToFloat3();
                m_scale_cached = m_scale_imgui = s.ToFloat();
            }
        }

        virtual void HandleExternalUpdate() override
        {
            Quaternion actualQ;
            VectorSIMD actualTranslation;
            float actualScale = 1.0f;

            // Get actual transform values
            //-------------------------------------------------------------------------

            if ( m_coreType == TypeIDCore::Transform )
            {
                auto const& transform = reinterpret_cast<Transform*>( m_pPropertyInstance );
                actualQ = transform->GetRotation();
                actualTranslation = transform->GetTranslation();
                actualScale = transform->GetScale();
            }
            else if ( m_coreType == TypeIDCore::Matrix )
            {
                auto const& matrix = reinterpret_cast<Matrix*>( m_pPropertyInstance );
                VectorSIMD vScale;
                matrix->Decompose( actualQ, actualTranslation, vScale );
                actualScale = vScale.ToFloat();
            }

            // Update the cached (and the imgui transform) when the actual is sufficiently different
            //-------------------------------------------------------------------------

            EulerAngles const currentRotation( m_rotation_cached );
            Quaternion const currentQ( currentRotation );

            Radians const angularDistance = Quaternion::Distance( currentQ, actualQ );
            if ( angularDistance > Degrees( 0.5f ) )
            {
                m_rotation_cached = actualQ.ToEulerAngles().GetAsDegrees();
                m_rotation_imgui = m_rotation_cached;
            }

            if ( !actualTranslation.IsNearEqual3( m_translation_cached ) )
            {
                m_translation_cached = actualTranslation.ToFloat3();
                m_translation_imgui = m_translation_cached;
            }

            if ( !Math::IsNearEqual( actualScale, m_scale_cached ) )
            {
                m_scale_cached = actualScale;
                m_scale_imgui = m_scale_cached;
            }
        }

    private:

        Float3                m_rotation_imgui;
        Float3                m_translation_imgui;
        float                 m_scale_imgui;

        Float3                m_rotation_cached;
        Float3                m_translation_cached;
        float                 m_scale_cached;
    };*/

    //-------------------------------------------------------------------------

/*    class ResourcePathEditor final : public PropertyEditor
    {
    public:

        ResourcePathEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
            , m_picker( context.m_pEditorContext->GetRawResourceDirectory() )
        {
            ResourcePathEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            return m_picker.UpdateAndDraw();
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_picker.GetPath();
            *reinterpret_cast<ResPath*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = *reinterpret_cast<ResPath*>( m_pPropertyInstance );
            m_picker.SetPath( m_value_cached );
        }

        virtual void HandleExternalUpdate() override
        {
            ResPath const* pActualPath = nullptr;
            pActualPath = reinterpret_cast<ResPath*>( m_pPropertyInstance );
            if ( *pActualPath != m_value_cached )
            {
                m_value_cached = *pActualPath;
                m_picker.SetPath( m_value_cached );
            }
        }

    private:

        ResourcePathPicker            m_picker;
        ResPath                  m_value_cached;
    };
    */

    //-------------------------------------------------------------------------
/*
    class ResourceIDEditor final : public PropertyEditor
    {
    public:

        ResourceIDEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
            , m_picker(*context.m_pEditorContext )
        {
            ResourceIDEditor::ResetWorkingCopy();
            if ( m_coreType == TypeIDCore::TResourcePtr )
            {
               m_picker.SetRequiredResourceType(Types::GetResourceInfoForType( propertyInfo.templateArgumentTypeID )->resourceTypeID );
            }
        }

        virtual bool InternalUpdateAndDraw() override
        {
            return m_picker.UpdateAndDraw();
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_picker.GetResourceID();

            if ( m_coreType == TypeIDCore::ResourceID )
            {
                *reinterpret_cast<ResourceID*>( m_pPropertyInstance ) = m_value_cached.IsValid() ? ResourceID( m_value_cached ) : ResourceID();
            }
            else if ( m_coreType == TypeIDCore::ResourcePtr || m_coreType == TypeIDCore::TResourcePtr )
            {
                *reinterpret_cast<ResPtr*>( m_pPropertyInstance ) = m_value_cached.IsValid() ? ResourceID( m_value_cached ) : ResourceID();
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == TypeIDCore::ResourceID )
            {
                m_value_cached = *reinterpret_cast<ResID*>( m_pPropertyInstance );
            }
            else if ( m_coreType == TypeIDCore::ResourcePtr || m_coreType == TypeIDCore::TResourcePtr )
            {
                m_value_cached = reinterpret_cast<ResPtr*>( m_pPropertyInstance )->GetResourceID();
            }

            m_picker.SetResourceID( m_value_cached );
        }

        virtual void HandleExternalUpdate() override
        {
            if ( m_coreType == TypeIDCore::ResourceID )
            {
                ResourceID* pResourceID = reinterpret_cast<ResID*>( m_pPropertyInstance );
                if ( *pResourceID != m_value_cached )
                {
                    m_value_cached = *pResourceID;
                }
            }
            else if ( m_coreType == TypeIDCore::ResourcePtr || m_coreType == TypeIDCore::TResourcePtr )
            {
                ResourceID resourceID = reinterpret_cast<ResPtr*>( m_pPropertyInstance )->GetResourceID();
                if ( resourceID != m_value_cached )
                {
                    m_value_cached = resourceID;
                }
            }

            m_picker.SetResourceID( m_value_cached );
        }

    private:

        ResourcePicker                          m_picker;
        ResID                              m_value_cached;
    };
    */

    //-------------------------------------------------------------------------

    /*class BitflagsEditor final : public PropertyEditor
    {
    public:

        BitflagsEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            BitflagsEditor::ResetWorkingCopy();
            ENGINE_ASSERT( m_coreType == TypeIDCore::BitFlags || m_coreType == TypeIDCore::TBitFlags );
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            if ( m_coreType == TypeIDCore::BitFlags )
            {
                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
                if ( ImGui::BeginTable( "FlagsTable", 9, ImGuiTableFlags_SizingFixedFit ) )
                {
                    constexpr static char const* const rowLabels[4] = { "00-07", "08-15", "16-23", "24-31" };

                    for ( uint8_t i = 0u; i < 4; i++ )
                    {
                        ImGui::TableNextRow();

                        for ( uint8_t j = 0u; j < 8; j++ )
                        {
                            uint8_t const flagIdx = i * 8 + j;
                            ImGui::TableNextColumn();

                            ImGui::PushID( &m_values_imgui[flagIdx] );
                            if ( ImGui::Checkbox( "##flag", &m_values_imgui[flagIdx] ) )
                            {
                                valueChanged = true;
                            }
                            ImGui::PopID();
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text( rowLabels[i] );
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
            else if ( m_coreType == TypeIDCore::TBitFlags )
            {
                // Get enum type for specific flags
                TypeID const enumTypeID = m_propertyInfo.templateArgumentTypeID;
                EnumInfo const* pEnumInfo = Types::GetEnumInfo( enumTypeID );
                ENGINE_ASSERT( pEnumInfo != nullptr );

                //-------------------------------------------------------------------------

                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
                if ( ImGui::BeginTable( "FlagsTable", 2, ImGuiTableFlags_SizingFixedFit ) )
                {
                    int32_t flagCount = 0;

                    // For each label 
                    for ( auto const& constant : pEnumInfo->constants )
                    {
                        if ( ( flagCount % 2 ) == 0 )
                        {
                            ImGui::TableNextRow();
                        }

                        ImGui::TableNextColumn();
                        flagCount++;

                        //-------------------------------------------------------------------------

                        int64_t const flagValue = constant.value;
                        ENGINE_ASSERT( flagValue >= 0 && flagValue <= 31 );
/*                        if ( ImGui::Checkbox( constant.id.c_str(), &m_values_imgui[flagValue] ) )
                        {
                            valueChanged = true;
                        }#1#
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }

            //-------------------------------------------------------------------------

            return valueChanged;
        }

        virtual void UpdatePropertyValue() override
        {
            auto pFlags = reinterpret_cast<BitFlags*>( m_pPropertyInstance );
            for ( uint8_t i = 0; i < 32; i++ )
            {
                pFlags->SetFlag( i, m_values_imgui[i] );
            }

            m_cachedFlags = pFlags->Get();
        }

        virtual void ResetWorkingCopy() override
        {
            auto pFlags = reinterpret_cast<BitFlags*>( m_pPropertyInstance );
            for ( uint8_t i = 0; i < 32; i++ )
            {
                m_values_imgui[i] = pFlags->IsFlagSet( i );
            }

            m_cachedFlags = pFlags->Get();
        }

        virtual void HandleExternalUpdate() override
        {
            auto pFlags = reinterpret_cast<BitFlags*>( m_pPropertyInstance );
            if ( pFlags->Get() != m_cachedFlags )
            {
                ResetWorkingCopy();
            }
        }

    private:

        bool        m_values_imgui[32];
        uint32_t    m_cachedFlags = 0;
    };*/

    //-------------------------------------------------------------------------

    class StringEditor final : public PropertyEditor
    {
    public:

        StringEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            StringEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
			ImGui::InputText( "##stringEd", m_Buffer, 256 );
            return ImGui::IsItemDeactivatedAfterEdit();
        }

        virtual void UpdatePropertyValue() override
        {
            *reinterpret_cast<String*>( m_pPropertyInstance ) = m_Buffer;
        }

        virtual void ResetWorkingCopy() override
        {
            String* pValue = reinterpret_cast<String*>( m_pPropertyInstance );
			m_Buffer = *pValue;
        }

        virtual void HandleExternalUpdate() override
        {
            String* pActualValue = reinterpret_cast<String*>( m_pPropertyInstance );
            if ((*pActualValue) != m_Buffer)
            {
				m_Buffer = *pActualValue;
            }
        }

    private:

		String m_Buffer;
    };

    //-------------------------------------------------------------------------

    class StringIDEditor final : public PropertyEditor
    {
        constexpr static uint32_t const s_bufferSize = 256;

    public:

        StringIDEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : m_buffer_cached("", s_bufferSize), m_IDString("", s_bufferSize), 
            PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            StringIDEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 60 );
            /*if ( ImGui::InputText( "##StringInput", m_buffer_imgui, s_bufferSize, ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                valueUpdated = true;
            }*/
            
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                valueUpdated = true;
            }

            if ( valueUpdated )
            {
//                StringTool::StripTrailingWhitespace( m_buffer_imgui );

				int length = StringUtils::Length(m_buffer_imgui);
				int i = length - 1;
				for (; i >= 0; --i)
				{
					if (m_buffer_imgui[i] != ' ')
					{
						break;
					}
				}
				Platform::MemoryClear(&m_buffer_imgui + (length - i), length - i);
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            ImGui::SetNextItemWidth( -1 );
            {
                GUI::ScopedFont const sf( GUI::Font::Tiny );
                ImGui::AlignTextToFramePadding();
//                ImGui::TextColored( Colors::LightGreen.ToFloat4(), m_IDString.Get() );
            }

            return valueUpdated;
        }

        virtual void UpdatePropertyValue() override
        {
            auto pValue = reinterpret_cast<StringID*>( m_pPropertyInstance );

            if (StringUtils::Length(m_buffer_imgui) > 0 )
            {
                m_buffer_cached = m_buffer_imgui;
                *pValue = StringID(m_buffer_imgui );
                m_IDString = String::Format(SE_TEXT("{:u}"), (uint32)pValue);
            }
            else
            {
                // pValue->Clear();
                m_buffer_cached.Clear();
                m_IDString = "Invalid";
            }
        }

        virtual void ResetWorkingCopy() override
        {
            /*auto pValue = reinterpret_cast<StringID*>( m_pPropertyInstance );
            if ( pValue->IsValid() )
            {
//				StringUtils::Copy(m_buffer_imgui, pValue->ToString(), 256)
//                strcpy_s( m_buffer_imgui, 256, pValue->c_str() );
//                m_buffer_cached = pValue->c_str();
                m_IDString = String::Format(SE_TEXT("{:u}"), pValue->ToUint() );
            }
            else
            {
                Platform::MemoryClear( m_buffer_imgui, s_bufferSize );
                m_buffer_cached.Clear();
                m_IDString = "Invalid";
            }*/
        }

        virtual void HandleExternalUpdate() override
        {
            /*StringID const* pActualValue = reinterpret_cast<StringID*>( m_pPropertyInstance );
            if ( pActualValue->IsValid() )
            {
//                if ( m_buffer_cached != pActualValue->c_str() )
//                {
//                    ResetWorkingCopy();
//                }
            }
            else // Invalid String ID
            {
                if ( !m_buffer_cached.IsEmpty() )
                {
                    ResetWorkingCopy();
                }
            }*/
        }

    private:

        Char                      m_buffer_imgui[s_bufferSize];
        String                    m_buffer_cached;
        String                    m_IDString;
    };


    //-------------------------------------------------------------------------
    /*
    class IntRangeEditor final : public PropertyEditor
    {
    public:

        IntRangeEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            IntRangeEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            constexpr float const verticalOffset = 3.0f;
            float const columnWidth = ImGui::GetColumnWidth();
            float const cursorStartPosX = ImGui::GetCursorScreenPos().x;

            //-------------------------------------------------------------------------

            bool isSet = m_value_imgui.IsSet();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1, 1 ) );
            if ( ImGui::Checkbox( "##isRangeSet", &isSet ) )
            {
                if ( isSet )
                {
                    m_value_imgui.m_begin = 0;
                    m_value_imgui.m_end = 1;
                }
                else
                {
                    m_value_imgui.Clear();
                }

                valueUpdated = true;
            }
            ImGui::PopStyleVar();
            ImGui::SameLine();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

            float const cursorEndPosX = ImGui::GetCursorScreenPos().x;
            float const inputWidth = ( columnWidth - ( cursorEndPosX - cursorStartPosX ) - ( ImGui::GetStyle().ItemSpacing.x * 2 ) ) / 2;
            int32_t tmpValue = 0;

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputScalar( "##min", ImGuiDataType_S32, isSet ? &m_value_imgui.m_begin : &tmpValue, 0, 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( true );
                valueUpdated = true;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputScalar( "##max", ImGuiDataType_S32, isSet ? &m_value_imgui.m_end : &tmpValue, 0, 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( false );
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            return valueUpdated;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<IntRange*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<IntRange*>( m_pPropertyInstance );

            // Ensure we always have a valid range!
            if ( m_value_imgui.IsSet() )
            {
                if ( m_value_imgui.IsValid() )
                {
                    m_value_imgui.Clear();
                    m_value_cached = m_value_imgui;
                }
            }
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<IntRange*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        // Ensure users cant input an invalid range
        void ValidateUserInput( bool wasRangeStartEdited )
        {
            if ( wasRangeStartEdited )
            {
                if ( m_value_imgui.m_begin > m_value_imgui.m_end )
                {
                    m_value_imgui.m_begin = m_value_imgui.m_end;
                }
            }
            else
            {
                if ( m_value_imgui.m_end < m_value_imgui.m_begin )
                {
                    m_value_imgui.m_end = m_value_imgui.m_begin;
                }
            }
        }

    private:

        IntRange m_value_imgui;
        IntRange m_value_cached;
    };
    */
    //-------------------------------------------------------------------------

    class FloatRangeEditor final : public PropertyEditor
    {
    public:

        FloatRangeEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {
            FloatRangeEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            constexpr float const verticalOffset = 3.0f;
            float const columnWidth = ImGui::GetColumnWidth();
            float const cursorStartPosX = ImGui::GetCursorScreenPos().x;

            //-------------------------------------------------------------------------

            bool isSet = m_value_imgui.IsSet();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1, 1 ) );
            if ( ImGui::Checkbox( "##isRangeSet", &isSet ) )
            {
                if ( isSet )
                {
                    m_value_imgui.begin = 0;
                    m_value_imgui.end = 1;
                }
                else
                {
                    m_value_imgui.Clear();
                }

                valueUpdated = true;
            }
            ImGui::PopStyleVar();
            ImGui::SameLine();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

            float const cursorEndPosX = ImGui::GetCursorScreenPos().x;
            float const inputWidth = ( columnWidth - ( cursorEndPosX - cursorStartPosX ) - ( ImGui::GetStyle().ItemSpacing.x * 2 ) ) / 2;
            float tmpValue = 0;

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputFloat( "##min", isSet ? &m_value_imgui.begin : &tmpValue, 0, 0, "%.3f", 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( true );
                valueUpdated = true;
            }
            ImGui::EndDisabled();
            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputFloat( "##max", isSet ? &m_value_imgui.end : &tmpValue, 0, 0, "%.3f", 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( false );
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            return valueUpdated;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<FloatRange*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<FloatRange*>( m_pPropertyInstance );

            // Ensure we always have a valid range!
            if ( m_value_imgui.IsSet() )
            {
                if ( m_value_imgui.IsValid() )
                {
                    m_value_imgui.Clear();
                    m_value_cached = m_value_imgui;
                }
            }
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<FloatRange*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        // Ensure users cant input an invalid range
        void ValidateUserInput( bool wasRangeStartEdited )
        {
            if ( wasRangeStartEdited )
            {
                if ( m_value_imgui.begin > m_value_imgui.end )
                {
                    m_value_imgui.begin = m_value_imgui.end;
                }
            }
            else
            {
                if ( m_value_imgui.end < m_value_imgui.begin )
                {
                    m_value_imgui.end = m_value_imgui.begin;
                }
            }
        }

    private:

        FloatRange m_value_imgui;
        FloatRange m_value_cached;
    };

    //-------------------------------------------------------------------------
    /*
    class FloatCurveEditor final : public PropertyEditor
    {
    public:

        FloatCurveEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
            , m_editor( m_value_imgui )
        {
            FloatCurveEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;
            if ( ImGui::BeginChild( "##Preview", ImVec2( ImGui::GetContentRegionAvail().x, 140 ) ) )
            {
                valueChanged = m_editor.UpdateAndDraw( true );
            }
            ImGui::EndChild();
            return valueChanged;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<FloatCurve*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            auto const& originalCurve = *reinterpret_cast<FloatCurve*>( m_pPropertyInstance );
            m_value_cached = m_value_imgui = originalCurve;

            m_editor.OnCurveExternallyUpdated();
            m_editor.ResetView();
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<FloatCurve*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
                m_editor.OnCurveExternallyUpdated();
            }
        }

    private:

        FloatCurve      m_value_imgui;
        FloatCurve      m_value_cached;
        CurveEditor     m_editor;
    };
    */
    //-------------------------------------------------------------------------
    // Factory
    //-------------------------------------------------------------------------

    ENGINE_GLOBAL_REGISTRY( PropertyGridEditorFactory );

    PropertyEditor* PropertyGridEditorFactory::TryCreateEditor( PropertyEditorContext const& context, TypeProperty const& propertyInfo, void* pPropertyInstance )
    {
        // ENGINE_ASSERT( propertyInfo.IsValid() );
        auto typeID = propertyInfo.typeID;
        // ENGINE_ASSERT( typeID.IsValid() );

        // Check if we have a custom editor for this type
        //-------------------------------------------------------------------------

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pCurrentFactory->SupportsTypeID( typeID ) )
            {
                return pCurrentFactory->TryCreateEditorInternal( context, propertyInfo, pPropertyInstance );
            }

            if ( pCurrentFactory->SupportsCustomEditorID( propertyInfo.customEditorID ) )
            {
                return pCurrentFactory->TryCreateEditorInternal( context, propertyInfo, pPropertyInstance );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        // Create core type editors
        //-------------------------------------------------------------------------

        if ( propertyInfo.IsEnumProperty() )
        {
            return New<EnumEditor>( context, propertyInfo, pPropertyInstance );
        }

        if ( IsCoreType( typeID ) )
        {
            TypeIDCore const coreType = GetCoreType( propertyInfo.typeID );
            switch ( coreType )
            {
                case TypeIDCore::Bool:
                {
                    return New<BoolEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Int8:
                {
                    return New<ScalarEditor<int8>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Int16:
                {
                    return New<ScalarEditor<int16_t>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Int32:
                {
                    return New<ScalarEditor<int32_t>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Int64:
                {
                    return New<ScalarEditor<int64_t>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Uint8:
                {
                    return New<ScalarEditor<uint8_t>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Uint16:
                {
                    return New<ScalarEditor<uint16_t>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Uint32:
                {
                    return New<ScalarEditor<uint32_t>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Uint64:
                {
                    return New<ScalarEditor<uint64_t>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Float:
                {
                    return New<ScalarEditor<float>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Double:
                {
                    return New<ScalarEditor<double>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Color:
                {
                    return New<ColorEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Quaternion:
                {
                    return nullptr; //New<RotationEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::UUID:
                {
                    return New<UUIDEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Microseconds:
                case TypeIDCore::Milliseconds:
                case TypeIDCore::Seconds:
                {
                    return nullptr;// New<TimeEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Matrix:
                case TypeIDCore::Transform:
                {
                    return nullptr; //New<TransformEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;
                /*case TypeIDCore::BitFlags:
                case TypeIDCore::TBitFlags:
                {
                    return New<BitflagsEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;*/

                case TypeIDCore::StringID:
                {
                    return New<StringIDEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::String:
                {
                    return New<StringEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Float2:
                {
                    return New<FloatNEditor<Float2>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Float3:
                {
                    return New<FloatNEditor<Float3>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::Float4:
                {
                    return New<FloatNEditor<Float4>>( context, propertyInfo, pPropertyInstance );
                }
                break;

                // case TypeIDCore::Percentage:
                // {
                //     return SE::New<PercentageEditor>( context, propertyInfo, pPropertyInstance );
                // }
                // break;

                // case TypeIDCore::IntRange:
                // {
                //     return SE::New<IntRangeEditor>( context, propertyInfo, pPropertyInstance );
                // }
                // break;

                case TypeIDCore::FloatRange:
                {
                    return New<FloatRangeEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                // case TypeIDCore::FloatCurve:
                // {
                //     return SE::New<FloatCurveEditor>( context, propertyInfo, pPropertyInstance );
                // }
                // break;

                default:
                {
                    return nullptr;
                }
                break;
            }
        }
/*
		if (typeID == Typeof<ResPath>())
		{
			return New<PG::ResourcePathEditor>( context, propertyInfo, pPropertyInstance );
		}


                case TypeIDCore::ResourceID:
                case TypeIDCore::ResourcePtr:
                case TypeIDCore::TResourcePtr:
                {
                    return New<PG::ResourceIDEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;

                case TypeIDCore::ResourceTypeID:
                {
                    return New<ResourceTypeIDEditor>( context, propertyInfo, pPropertyInstance );
                }
                break;
                */

        return nullptr;
    }
}