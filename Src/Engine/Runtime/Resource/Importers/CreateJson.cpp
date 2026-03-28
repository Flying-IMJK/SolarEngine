
#include "CreateJson.h"

#include "Runtime/Resource/AssetContent.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"
#include "Core/Serialization/JsonWriters.hpp"
#include "rapidjson/stringbuffer.h"
#include "Runtime/Resource/AssetsCache.h"
#include "Runtime/Resource/Storage/JsonStorageProxy.h"

namespace SE
{
    bool CreateJson::Create(const StringView& path, Json::StringBuffer& data, const TypeID& dataTypeID)
    {
        StringAnsiView data1((char*)data.GetString(), (int32)data.GetSize());
        return Create(path, data1, dataTypeID);
    }

    bool CreateJson::Create(const StringView& path, const StringAnsiView& data, const TypeID& dataTypeID)
    {
        UID id = UID::New();

        LOG_INFO("Resource", "Creating json resource of type \'{1}\' at \'{0}\'", path, dataTypeID.ToString());

        // Try use the same asset ID
        if (FileSystem::FileExists(path))
        {
            TypeID typeID;
            JsonStorageProxy::GetAssetInfo(path, id, typeID);
            if (typeID != dataTypeID)
            {
                LOG_WARNING("Resource", "Asset will have different type name {0} -> {1}", typeID, dataTypeID);
            }
        }
        else
        {
            const String directory = FileSystem::GetDirectoryName(path);
            if (!FileSystem::DirectoryExists(directory))
            {
                if (FileSystem::CreateDirectory(directory))
                {
                    LOG_WARNING("Resource", "Failed to create directory '{}'", directory);
                    return true;
                }
            }
        }

        Json::StringBuffer buffer;

        // Serialize to json
        PrettyJsonWriter writerObj(buffer);
        JsonWriter& writer = writerObj;
        writer.StartObject();
        {
            // Json resource header
            writer.JKEY("ID");
            writer.UUID(id);
            writer.JKEY("TypeTypeID");
            writer.Uint(dataTypeID);

            // Json resource data
            writer.JKEY("Data");
            writer.RawValue(data.Get(), data.Length());
        }
        writer.EndObject();

        // Save json to file
        if (File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
        {
            LOG_WARNING("Resource", "Failed to save json to file");
            return true;
        }

        // Reload asset at the target location if is loaded
        auto asset = AssetContent::GetAsset(path);
        if (asset)
        {
            asset->Reload();
        }
        else
        {
            AssetContent::GetRegistry()->RegisterAsset(id, dataTypeID, path);
        }

        return false;
    }

    void FormatPoValue(String& value)
    {
        value.Replace(SE_TEXT("\\n"), SE_TEXT("\n"));
        value.Replace(SE_TEXT("%s"), SE_TEXT("{}"));
        value.Replace(SE_TEXT("%d"), SE_TEXT("{}"));
    }

    CreateAssetResult CreateJson::ImportPo(CreateAssetContext& context)
    {
        return CreateAssetResult::Error;

        /*// Base
        IMPORT_SETUP(LocalizedStringTable, 1);

        // Load file (UTF-16)
        String inputData;
        if (File::ReadAllText(context.InputPath, inputData))
        {
            return CreateAssetResult::InvalidPath;
        }

        // Use virtual asset for data storage and serialization
        AssetRef<LocalizedStringTable> asset = AssetContent::CreateVirtualAsset<LocalizedStringTable>();
        if (!asset)
            return CreateAssetResult::Error;

        // Parse PO format
        int32 pos = 0;
        int32 pluralCount = 0;
        int32 lineNumber = 0;
        bool fuzzy = false, hasNewContext = false;
        StringView msgctxt, msgid;
        String idTmp;
        while (pos < inputData.Length())
        {
            // Read line
            const int32 startPos = pos;
            while (pos < inputData.Length() && inputData[pos] != '\n')
                pos++;
            const StringView line(&inputData[startPos], pos - startPos);
            lineNumber++;
            pos++;
            const int32 valueStart = line.Find('\"') + 1;
            const int32 valueEnd = line.FindLast('\"');
            const StringView value(line.Get() + valueStart, Math::Max(valueEnd - valueStart, 0));

            if (line.StartsWith(StringView(SE_TEXT("msgid_plural"))))
            {
                // Plural form
            }
            else if (line.StartsWith(StringView(SE_TEXT("msgid"))))
            {
                // Id
                msgid = value;

                // Reset context if already used
                if (!hasNewContext)
                    msgctxt = StringView();
                hasNewContext = false;
            }
            else if (line.StartsWith(StringView(SE_TEXT("msgstr"))))
            {
                // String
                if (msgid.HasChars())
                {
                    // Format message
                    String msgstr(value);
                    FormatPoValue(msgstr);

                    // Get message id
                    StringView id = msgid;
                    if (msgctxt.HasChars())
                    {
                        idTmp = String(msgctxt) + SE_TEXT(".") + String(msgid);
                        id = idTmp;
                    }

                    int32 indexStart = line.Find('[');
                    if (indexStart != -1 && indexStart < valueStart)
                    {
                        indexStart++;
                        while (indexStart < line.Length() && StringUtils::IsWhitespace(line[indexStart]))
                            indexStart++;
                        int32 indexEnd = line.Find(']');
                        while (indexEnd > indexStart && StringUtils::IsWhitespace(line[indexEnd - 1]))
                            indexEnd--;
                        int32 index = -1;
                        StringUtils::Parse(line.Get() + indexStart, (uint32)(indexEnd - indexStart), &index);
                        if (pluralCount <= 0)
                        {
                            LOG_ERROR("Resource", "Missing 'nplurals'. Cannot use plural message at line {0}", lineNumber);
                            return CreateAssetResult::Error;
                        }
                        if (index < 0 || index > pluralCount)
                        {
                            LOG_ERROR("Resource", "Invalid plural message index at line {0}", lineNumber);
                            return CreateAssetResult::Error;
                        }

                        // Plural message
                        asset->AddPluralString(id, msgstr, index);
                    }
                    else
                    {
                        // Message
                        asset->AddString(id, msgstr);
                    }
                }
            }
            else if (line.StartsWith(StringView(SE_TEXT("msgctxt"))))
            {
                // Context
                msgctxt = value;
                hasNewContext = true;
            }
            else if (line.StartsWith('\"'))
            {
                // Config
                const Char* pluralForms = StringUtils::Find(line.Get(), SE_TEXT("Plural-Forms"));
                if (pluralForms != nullptr && pluralForms < line.Get() + line.Length() - 1)
                {
                    // Process plural forms rule
                    const Char* nplurals = StringUtils::Find(pluralForms, SE_TEXT("nplurals"));
                    if (nplurals && nplurals < line.Get() + line.Length())
                    {
                        while (*nplurals && *nplurals != '=')
                            nplurals++;
                        while (*nplurals && (StringUtils::IsWhitespace(*nplurals) || *nplurals == '='))
                            nplurals++;
                        const Char* npluralsStart = nplurals;
                        while (*nplurals && !StringUtils::IsWhitespace(*nplurals) && *nplurals != ';')
                            nplurals++;
                        StringUtils::Parse(npluralsStart, (uint32)(nplurals - npluralsStart), &pluralCount);
                        if (pluralCount < 0 || pluralCount > 100)
                        {
                            LOG_ERROR("Resource", "Invalid 'nplurals' at line {0}", lineNumber);
                            return CreateAssetResult::Error;
                        }
                    }
                    // TODO: parse plural forms rule
                }
                const Char* language = StringUtils::Find(line.Get(), SE_TEXT("Language"));
                if (language != nullptr && language < line.Get() + line.Length() - 1)
                {
                    // Process language locale
                    while (*language && *language != ':')
                        language++;
                    language++;
                    while (*language && StringUtils::IsWhitespace(*language))
                        language++;
                    const Char* languageStart = language;
                    while (*language && !StringUtils::IsWhitespace(*language) && *language != '\\' && *language != '\"')
                        language++;
                    asset->Locale.Set(languageStart, (int32)(language - languageStart));
                    if (asset->Locale == SE_TEXT("English"))
                        asset->Locale = SE_TEXT("en");
                    if (asset->Locale.Length() > 5)
                        LOG_WARNING("Resource", "Imported .po file uses invalid locale '{0}'", asset->Locale);
                }
            }
            else if (line.StartsWith('#') || line.IsEmpty())
            {
                // Comment
                const Char* fuzzyPos = StringUtils::Find(line.Get(), SE_TEXT("fuzzy"));
                fuzzy |= fuzzyPos != nullptr && fuzzyPos < line.Get() + line.Length() - 1;
            }
        }
        if (asset->Locale.IsEmpty())
            LOG_WARNING("Resource", "Imported .po file has missing locale");

        // Save asset
        return asset->Save(context.TargetAssetPath) ? CreateAssetResult::CannotSaveFile : CreateAssetResult::Ok;*/
    }
} // SE