#pragma once


namespace SE::BuildTool
{
    template<typename K, typename V>
    class Dictionary
    {
    public:
        struct Entry
        {
            K Key;
            V Value;
        };

        using Storage = std::vector<Entry>;
        using iterator = typename Storage::iterator;
        using const_iterator = typename Storage::const_iterator;

        V& operator[](const K& key)
        {
            auto it = Find(key);
            if (it == end())
            {
                m_entries.push_back({key, V()});
                return m_entries.back().Value;
            }
            return it->Value;
        }

        iterator Find(const K& key)
        {
            return std::find_if(m_entries.begin(), m_entries.end(), [&](const Entry& entry) {
                return entry.Key == key;
            });
        }

        const_iterator Find(const K& key) const
        {
            return std::find_if(m_entries.begin(), m_entries.end(), [&](const Entry& entry) {
                return entry.Key == key;
            });
        }

        iterator begin() { return m_entries.begin(); }
        iterator end() { return m_entries.end(); }
        const_iterator begin() const { return m_entries.begin(); }
        const_iterator end() const { return m_entries.end(); }

        void Clear() { m_entries.clear(); }
        int Count() const { return (int)m_entries.size(); }

    private:
        Storage m_entries;
    };
}

