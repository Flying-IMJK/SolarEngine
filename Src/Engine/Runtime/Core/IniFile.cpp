#include "IniFile.h"
#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/Serialization/FileWriteStream.h"
#include "Runtime/ThirdParty/iniparser/iniparser.h"

//-------------------------------------------------------------------------

namespace SE
{
	#define ASCIILINESZ 1024
	void SaveIniSection(const _dictionary_ * d, const char * s, FileWriteStream* stream)
	{
		int     j ;
		char    keym[ASCIILINESZ+1];
		int     seclen ;

		if (d == NULL) return ;
		if (! iniparser_find_entry(d, s)) return ;

		seclen  = (int)strlen(s);
		stream->Write(StringAnsi::Format("\n[{0}]\n", s));
		sprintf(keym, "%s:", s);
		for (j=0 ; j<d->size ; j++) {
			if (d->key[j]==NULL)
				continue ;
			if (!strncmp(d->key[j], keym, seclen+1)) {
				stream->Write(StringAnsi::Format("{:<30} = {1}\n",
					d->key[j]+seclen+1,
					d->val[j] ? d->val[j] : ""));
/*				fprintf(f,
					"%-30s = %s\n",
					d->key[j]+seclen+1,
					d->val[j] ? d->val[j] : "");*/
			}
		}
		stream->Write(StringAnsiView("\n"));
		return ;
	}

	void SaveIni(_dictionary_ *d, FileWriteStream* stream)
	{
		int          i ;
		int          nsec ;
		const char * secname ;

		nsec = iniparser_getnsec(d);
		if (nsec<1)
		{
			/* No section in file: dump all keys as they are */
			for (i=0 ; i<d->size ; i++)
			{
				if (d->key[i]==NULL)
					continue ;

				stream->Write(StringAnsi::Format("{0} = {1}\n", d->key[i], d->val[i]));
			}
			return;
		}
		else
		{
			for (i=0 ; i<nsec ; i++) {
				secname = iniparser_getsecname(d, i) ;
				SaveIniSection(d, secname, stream);
//				iniparser_dumpsection_ini(d, secname, f);
			}
			stream->Write(StringAnsiView("\n"));
		}
	}

	const char* strlwc(const char * in, char *out, unsigned len)
	{
		unsigned i ;

		if (in==NULL || out == NULL || len==0) return NULL ;
		i=0 ;
		while (in[i] != '\0' && i < len-1) {
			out[i] = (char)StringUtils::ToLower(in[i]);
			i++ ;
		}
		out[i] = '\0';
		return out ;
	}

	const String dictionary_get(const _dictionary_ * d, const char * key, const Char * def)
	{
		unsigned    hash ;
		size_t      i ;

		hash = dictionary_hash(key);
		for (i=0 ; i<d->size ; i++) {
			if (d->key[i]==NULL)
				continue ;
			/* Compare hash */
			if (hash==d->hash[i]) {
				/* Compare string, to avoid hash collisions */
				if (!StringUtils::Compare(key, d->key[i])) {
					return String(d->val[i]);
				}
			}
		}
		return String(def) ;
	}

	const String GetString(const _dictionary_ * d, const char * key, const Char * def)
	{
		const char * lc_key ;
		const char * sval ;
		char tmp_str[ASCIILINESZ+1];

		if (d==NULL || key==NULL)
			return String(def) ;

		lc_key =  strlwc(key, tmp_str, sizeof(tmp_str));
		return dictionary_get(d, lc_key, def);
	}

    IniFile::IniFile(StringView const &filePath)
    {
        ENGINE_ASSERT(!filePath.IsEmpty());
        m_pDictionary = iniparser_load(filePath.ToStringAnsi().GetText());
    }

    IniFile::IniFile()
    {
        m_pDictionary = dictionary_new(0);
    }

    IniFile::~IniFile()
    {
        if (m_pDictionary != nullptr)
        {
            iniparser_freedict(m_pDictionary);
        }
    }

    bool IniFile::HasEntry(char const *key) const
    {
        ENGINE_ASSERT(IsValid());
        return iniparser_find_entry(m_pDictionary, key) > 0;
    }

    bool IniFile::SaveToFile(StringView const &filePath) const
    {
        ENGINE_ASSERT(IsValid());

		FileWriteStream* fileWriteStream = FileWriteStream::Open(filePath);
        bool const wasFileOpened = fileWriteStream != nullptr;
        if (wasFileOpened)
        {
			SaveIni(m_pDictionary, fileWriteStream);
			fileWriteStream->Close();
//            iniparser_dump_ini(m_pDictionary, pFile);
        }

        return wasFileOpened;
    }

    bool IniFile::TryGetBool(char const *key, bool &outValue) const
    {
        ENGINE_ASSERT(IsValid());

        if (HasEntry(key))
        {
            outValue = (bool)iniparser_getboolean(m_pDictionary, key, false);
            return true;
        }

        return false;
    }

    bool IniFile::TryGetInt(char const *key, int32 &outValue) const
    {
        ENGINE_ASSERT(IsValid());

        if (HasEntry(key))
        {
            outValue = iniparser_getint(m_pDictionary, key, 0);
            return true;
        }

        return false;
    }

    bool IniFile::TryGetUInt(char const *key, uint32 &outValue) const
    {
        ENGINE_ASSERT(IsValid());

        if (HasEntry(key))
        {
            outValue = (uint32)iniparser_getint(m_pDictionary, key, 0);
            return true;
        }

        return false;
    }

    bool IniFile::TryGetString(char const *key, String &outValue) const
    {
        ENGINE_ASSERT(IsValid());

        if (HasEntry(key))
        {
            outValue = iniparser_getstring(m_pDictionary, key, "");
            return true;
        }

        return false;
    }

    bool IniFile::TryGetFloat(char const *key, float &outValue) const
    {
        ENGINE_ASSERT(IsValid());

        if (HasEntry(key))
        {
            outValue = (float)iniparser_getdouble(m_pDictionary, key, 0.0f);
            return true;
        }

        return false;
    }

    bool IniFile::GetBoolOrDefault(char const *key, bool defaultValue) const
    {
        ENGINE_ASSERT(IsValid());
        return (bool)iniparser_getboolean(m_pDictionary, key, defaultValue);
    }

    int32 IniFile::GetIntOrDefault(char const *key, int32 defaultValue) const
    {
        ENGINE_ASSERT(IsValid());
        return (int32)iniparser_getint(m_pDictionary, key, defaultValue);
    }

    uint32 IniFile::GetUIntOrDefault(char const *key, uint32 defaultValue) const
    {
        ENGINE_ASSERT(IsValid());
        return (uint32)iniparser_getint(m_pDictionary, key, defaultValue);
    }

	StringAnsi IniFile::GetStringOrDefault(char const *key, StringAnsi const &defaultValue) const
    {
        ENGINE_ASSERT(IsValid());
        return iniparser_getstring(m_pDictionary, key, defaultValue.Get());
    }

	StringAnsi IniFile::GetStringOrDefault(char const *key, char const *pDefaultValue) const
    {
        ENGINE_ASSERT(IsValid());
        return StringAnsi(iniparser_getstring(m_pDictionary, key, pDefaultValue));
    }

	String IniFile::GetStringOrDefault(char const *key, String const &defaultValue) const
	{
		ENGINE_ASSERT(IsValid());
		return String(GetString(m_pDictionary, key, defaultValue.Get())) ;
	}

	String IniFile::GetStringOrDefault(char const *key, Char const *pDefaultValue) const
	{
		ENGINE_ASSERT(IsValid());
		return GetString(m_pDictionary, key, pDefaultValue);
	}

    float IniFile::GetFloatOrDefault(char const *key, float defaultValue) const
    {
        ENGINE_ASSERT(IsValid());
        return (float)iniparser_getdouble(m_pDictionary, key, defaultValue);
    }

    void IniFile::CreateSection(char const *section)
    {
        ENGINE_ASSERT(IsValid());
        iniparser_set(m_pDictionary, section, nullptr);
    }

    void IniFile::SetBool(char const *key, bool value)
    {
        ENGINE_ASSERT(IsValid());
        iniparser_set(m_pDictionary, key, value ? "True" : "False");
    }

    void IniFile::SetInt(char const *key, int32 value)
    {
        ENGINE_ASSERT(IsValid());
		StringAnsi buffer = StringAnsi::Format("{:d}", value);
        iniparser_set(m_pDictionary, key, buffer.Get());
    }

    void IniFile::SetUInt(char const *key, uint32 value)
    {
        ENGINE_ASSERT(IsValid());
		StringAnsi buffer = StringAnsi::Format("{:d}", value);
        iniparser_set(m_pDictionary, key, buffer.Get());
    }

    void IniFile::SetFloat(char const *key, float value)
    {
        ENGINE_ASSERT(IsValid());
		StringAnsi buffer = StringAnsi::Format("{:d}", value);
        iniparser_set(m_pDictionary, key, buffer.Get());
    }

    void IniFile::SetString(char const *key, char const *pString)
    {
        ENGINE_ASSERT(IsValid());
        iniparser_set(m_pDictionary, key, pString);
    }

    void IniFile::SetString(char const *key, StringAnsi const &string)
    {
        ENGINE_ASSERT(IsValid());
        iniparser_set(m_pDictionary, key, string.Get());
    }

	void IniFile::SetString(char const *key, Char const *pString)
	{
		SetString(key, StringAnsi(pString));
	}

	void IniFile::SetString(char const *key, String const &string)
	{
		SetString(key, StringAnsi(string));
	}
}