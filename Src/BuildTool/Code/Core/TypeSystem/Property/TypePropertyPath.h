#pragma once


namespace SE::BuildTool
{
    class TypePropertyPath
    {
    public:
        struct Element
        {
            StringID propertyID = StringID::Invalid;
        };

        TypePropertyPath() = default;
        explicit TypePropertyPath(StringID rootID)
        {
            m_elements.push_back({rootID});
        }

        size_t GetNumElements() const { return m_elements.size(); }
        const Element& operator[](size_t index) const { return m_elements[index]; }
        StringID GetRootID() const { return m_elements.empty() ? StringID::Invalid : m_elements.front().propertyID; }

    private:
        std::vector<Element> m_elements;
    };
}
