#pragma once
#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Types/Strings/String.h"


//-------------------------------------------------------------------------

struct _dictionary_;


namespace SE
{
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME IniFile
    {

    public:
        IniFile();
        IniFile(StringView const &filePath);
        ~IniFile();

        IniFile &operator=(IniFile const &) = default;

        inline bool IsValid() const { return m_pDictionary != nullptr; }
        bool SaveToFile(StringView const &filePath) const;
        bool HasEntry(char const *key) const;

        //-------------------------------------------------------------------------

        bool TryGetBool(char const *key, bool &outValue) const;
        bool TryGetInt(char const *key, int32 &outValue) const;
        bool TryGetUInt(char const *key, uint32 &outValue) const;
        bool TryGetString(char const *key, String &outValue) const;
        bool TryGetFloat(char const *key, float &outValue) const;

        bool GetBoolOrDefault(char const *key, bool defaultValue) const;
        int32 GetIntOrDefault(char const *key, int32 defaultValue) const;
        uint32 GetUIntOrDefault(char const *key, uint32 defaultValue) const;
        StringAnsi GetStringOrDefault(char const *key, StringAnsi const &defaultValue) const;
		StringAnsi GetStringOrDefault(char const *key, char const *pDefaultValue) const;
		String GetStringOrDefault(char const *key, String const &defaultValue) const;
		String GetStringOrDefault(char const *key, Char const *pDefaultValue) const;
        float GetFloatOrDefault(char const *key, float defaultValue) const;

        //-------------------------------------------------------------------------

        void CreateSection(char const *section);
        void SetBool(char const *key, bool value);
        void SetInt(char const *key, int32 value);
        void SetUInt(char const *key, uint32 value);
        void SetFloat(char const *key, float value);
        void SetString(char const *key, char const *pString);
        void SetString(char const *key, StringAnsi const &string);
		void SetString(char const *key, Char const *pString);
		void SetString(char const *key, String const &string);

    private:
        _dictionary_ *m_pDictionary = nullptr;
    };
}