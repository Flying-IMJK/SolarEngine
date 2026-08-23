#pragma once

#include "ReflectionProjectTypes.h"

//-------------------------------------------------------------------------
namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Access level for API types and members
    // -------------------------------------------------------------------------

    enum class AccessLevel
    {
        Private,
        Protected,
        Public,
        Internal,
    };

    // -------------------------------------------------------------------------
    // Type mapping table (C++ <-> C#)
    // -------------------------------------------------------------------------

    struct TypeMapping
    {
        const char* cppType;
        const char* csType;
        const char* csInterop;
        bool        isBlittable;
        bool        isString;
        bool        isObject;
    };

    // -------------------------------------------------------------------------
    // Binding extension for DataType
    // -------------------------------------------------------------------------

    enum class PropertyFlags
    {
        None           = 0,
        IsStructure    = 1 << 0,
        IsEnum         = 1 << 1,
        IsBitFlags     = 1 << 2,
        IsArray        = 1 << 3,
        IsDynamicArray = 1 << 4,
    };

    class PropertyPath
    {
    public:
        struct Element
        {
            StringID propertyID = StringID::Invalid;
        };

        PropertyPath() = default;
        explicit PropertyPath(StringID rootID) { m_elements.push_back({rootID}); }

        size_t         GetNumElements() const { return m_elements.size(); }
        const Element& operator[](size_t index) const { return m_elements[index]; }
        StringID GetRootID() const { return m_elements.empty() ? StringID::Invalid : m_elements.front().propertyID; }

    private:
        std::vector<Element> m_elements;
    };

    struct PropertyData
    {
    public:
        PropertyData() = default;

        PropertyData(std::string const& name, int32_t lineNumber) : propertyID(name), name(name), lineNumber(lineNumber)
        {}

        PropertyData(std::string const& name, std::string const& typeName, int32_t lineNumber) :
            propertyID(name), name(name), typeName(typeName), lineNumber(lineNumber)
        {}

        inline bool IsStructureProperty() const { return flags.IsFlag(PropertyFlags::IsStructure); }
        inline bool IsEnumProperty() const { return flags.IsFlag(PropertyFlags::IsEnum); }
        inline bool IsBitFlagsProperty() const { return flags.IsFlag(PropertyFlags::IsBitFlags); }
        inline bool IsArrayProperty() const
        {
            return flags.IsFlag(PropertyFlags::IsArray) || flags.IsFlag(PropertyFlags::IsDynamicArray);
        }
        inline bool IsStaticArrayProperty() const
        {
            return flags.IsFlag(PropertyFlags::IsArray) && !flags.IsFlag(PropertyFlags::IsDynamicArray);
        }
        inline bool     IsDynamicArrayProperty() const { return flags.IsFlag(PropertyFlags::IsDynamicArray); }
        inline uint32_t GetArraySize() const
        {
            ENGINE_ASSERT(arraySize > 0);
            return (uint32_t)arraySize;
        }

        inline bool operator==(PropertyData const& RHS) const { return propertyID == RHS.propertyID; }
        inline bool operator!=(PropertyData const& RHS) const { return propertyID != RHS.propertyID; }

        // Dev Info
        //-------------------------------------------------------------------------

        std::string      GetFriendlyName() const;
        std::string_view GetCategory() const { return category; }

        // MetaData
        //-------------------------------------------------------------------------

        bool HasMetaData() const { return !metaData.empty(); }

    public:
        TypeID                   propertyID;
        int                      lineNumber = -1;
        TypeID                   typeID;
        std::string              name;
        std::string              metaData;
        std::string              description;
        std::string              typeName;
        std::string              templateArgTypeName;
        int                      arraySize = -1;
        EnumFlags<PropertyFlags> flags;
        bool                     isDevOnly = true;

        // From MetaData
        std::string category;
        bool        isToolsReadOnly      = false;
        bool        showInRestrictedMode = false;
    };

    //-------------------------------------------------------------------------
    struct TypeInfoParam
    {
        TypeID      type;
        std::string name;
        // For a native C-style array parameter synthesized from a reflected field.
        // Function declarations use the type spelling directly, while field
        // accessors keep the extent separately in TypeInfoField.
        int         arraySize = 0;
        bool        isPointer = false;
        bool        isConst   = false;
        bool        isRef     = false;
        bool        isOut     = false;
        std::string defaultValue;
        std::string attributes;
        std::string marshalAs;
        std::string comment;
    };


    struct TypeInfoBase
    {
        enum class Flag
        {
            Unknown,
            IsStruct,
            IsEnum,
            IsMeta,
        };

    public:
        TypeInfoBase() = default;
        virtual ~TypeInfoBase() = default;

        TypeInfoBase(StringID typeID, std::string const& name, Flag flag) : typeID(typeID), name(name), flag(flag) {}

        bool IsFlag(Flag flag) const { return this->flag == flag; }
        Flag GetFlag() const { return flag; }

        // Dev tools helpers
        std::string GetFriendlyName() const;
        std::string GetCategory() const;

    public:
        bool                     isReflect = false;
        bool                     isAPI     = false;
        TypeID                   typeID;
        HeaderID                 headerID;
        std::string              name = "Invalid";
        std::vector<std::string> namespaceScopeList;
        std::vector<std::string> structScopeList;

        bool isScriptingObject = false;
        std::string assemblyName;
        std::string assemblyDir;
        std::string comment;

        bool    isDevOnly = false;
    private:
        Flag flag = Flag::Unknown;

    };


    struct TypeInfoFunc
    {
        std::string                name;
        TypeID                     returnType;
        std::vector<TypeInfoParam> params;
        // Native C-style array extent for synthesized field getters. This is
        // deliberately separate from returnType because C++ cannot return an
        // array by value.
        int returnArraySize = 0;

        bool isReflect = false;
        bool isAPI     = false;

        bool        isStatic  = false;
        bool        isVirtual = false;
        bool        isConst   = false;
        bool        APINoProxy   = false;
        bool        APIIsSealed  = false;
        bool        APIIsStatic  = false;
        bool        APIIsPropertie = false;

        std::string uniqueName;
        std::string entryPoint;

        AccessLevel access       = AccessLevel::Public;
        std::string attributes;
        std::string comment;
        std::string marshalAs;
        int         lineNumber = -1;
    };

    struct TypeInfoEvent
    {
        bool isReflect = false;
        bool isAPI     = false;

        std::string                name;
        TypeID                     cppType;
        std::vector<TypeInfoParam> params;
        bool                       isStatic = false;
        AccessLevel                access   = AccessLevel::Public;
        std::string                attributes;
        std::string                comment;
        int                        lineNumber = -1;
    };

    struct TypeInfoField
    {
        bool isAPI    = false;
        bool isStatic = false;

        TypeID type;
        std::string name;

        bool isReflect = false;

        bool        APIIsReadOnly   = false;

        std::string attributes;
        std::string defaultValue;
        std::string comment;
        std::string marshalAs;
        int         arraySize  = 0;
        int         lineNumber = -1;
    };

    struct TypeInfoStruct : TypeInfoBase
    {
        StringID                   parentTypeID;
        std::string                baseClassName;
        std::vector<TypeInfoStruct*> interfaces;
        std::vector<TypeInfoField> fields;
        std::vector<TypeInfoFunc>  functions;
        std::vector<TypeInfoEvent> events;

        bool isStruct          = false;
        bool isAbstract        = false;
        bool isPod             = false;
        bool isTemplateInstantiation = false;
        std::string templateInstantiationTypeName;

        bool        APIIsAbstract    = false;
        bool        APIIsSealed      = false;
        bool        APIIsStatic      = false;
        bool        APINoSpawn       = false;
        bool        APINoConstructor = false;
        bool        APIIsInterface   = false;
        bool        APIIsNativeInvokeUseName = false;
        std::string APIName;
        std::string APIAttributes;
        std::string APIMarshalAs;
        // Existing managed type used by API bindings instead of generating a
        // C# declaration for this native type.
        std::string APIInBuildMapType;

        TypeInfoStruct(StringID typeID, std::string const& name) : TypeInfoBase(typeID, name, Flag::IsStruct) {};

        bool                HasProperties() const { return !fields.empty(); }
        PropertyData const* GetPropertyDescriptor(StringID propertyID) const;

        bool HasArrayProperties() const;
        bool HasDynamicArrayProperties() const;
        bool HasResourcePtrProperties() const;
        bool HasResourcePtrOrStructProperties() const;
    };

    //-------------------------------------------------------------------------

    struct EnumDataConstant
    {
        StringID    ID;
        std::string label;
        int         value;
        std::string description;
    };

    struct TypeInfoEnum : TypeInfoBase
    {
        Utils::TypeIDCore             underlyingType = Utils::TypeIDCore::Uint8;
        std::vector<EnumDataConstant> enumConstants;
        std::string                   APIAttributes;

        TypeInfoEnum(StringID typeID, std::string const& name) : TypeInfoBase(typeID, name, Flag::IsEnum) {};

        // Enum functions
        void AddEnumConstant(EnumDataConstant const& constant);
        bool IsValidEnumLabelID(StringID labelID) const;
        bool GetValueFromEnumLabel(StringID labelID, uint32& value) const;
    };

    enum class InjectEnum
    {
        CPP,
        CS
    };

    struct TypeInfoInjectedCode
    {
        HeaderID    headID;
        InjectEnum  lang;
        std::string code;
        int         lineNumber = -1;
    };

    //-------------------------------------------------------------------------

    struct ReflectedResourceType
    {
        // Fill the resource type ID and the friendly name from the macro registration string
        bool TryParseResourceRegistrationMacroString(std::string const& registrationStr);

    public:
        TypeID                typeID;
        TypeID                resourceTypeID;
        std::string           friendlyName;
        HeaderID              headerID;
        std::string           className;
        std::string           namespaceName;
        std::vector<StringID> parents;
    };
} // namespace SE::BuildTool
