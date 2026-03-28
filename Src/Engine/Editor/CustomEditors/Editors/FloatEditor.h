#pragma once

#include "Editor/CustomEditors/CustomEditor.h"

namespace SE::Editor
{
    class IFloatValueElement;

    /// <summary>
    /// Default implementation of the inspector used to edit float value type properties.
    /// </summary>
    class FloatEditor : CustomEditor
    {
    private:
        IFloatValueElement* m_Element;

        void OnValueChanged();

    protected:
        void OnInitialize(LayoutElementsContainer* layout) override;

    public:
        void RefreshInternal() override;
    };
} // SE
