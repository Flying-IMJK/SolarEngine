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

    struct TypeInfo
    {
        TypeID      typeID;
        int         arraySize = 0;
        int         pointerDepth = 0;
        bool        isPointer = false;
        bool        isConst   = false;
        bool        isRef     = false;
        bool        isMoveRef = false;

        TypeInfo() = default;
        TypeInfo(const TypeInfo& other);
        TypeInfo(const TypeID& other);

        // Template arguments are the sole generic representation.
        std::vector<TypeInfo> genericityArgs;

        std::string ToNativeType() const;

        std::string ToString(bool includeArray = true, bool useGlobal = false) const;

        static TypeInfo Void;
    };

    enum class ApiParameterDirection
    {
        In,
        Ref,
        Out,
    };

    struct TypeInfoParam
    {
        TypeInfo    type;
        std::string name;
        ApiParameterDirection direction = ApiParameterDirection::In;

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
            IsClassStruct,
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
        TypeInfo              returnType;
        std::vector<TypeInfoParam> params;

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
        TypeInfo                   cppType;
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

        TypeInfo type;
        std::string name;

        bool isReflect = false;

        bool        APIIsReadOnly   = false;

        std::string attributes;
        std::string defaultValue;
        std::string comment;
        std::string marshalAs;
        int         lineNumber = -1;

        bool IsReflectedProperty() const { return isReflect && !isStatic; }
        bool IsStaticArray() const { return type.arraySize > 0; }
        TypeID GetPropertyID() const { return TypeID(name); }
        std::string GetFriendlyName() const;
    };

    struct TypeInfoStruct : TypeInfoBase
    {
        TypeID                          parentTypeID;
        std::string                     baseClassName;
        std::vector<TypeInfoStruct*>    interfaces;
        std::vector<TypeInfoField>      fields;
        std::vector<TypeInfoFunc>       functions;
        std::vector<TypeInfoEvent>      events;

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

        TypeInfoStruct(StringID typeID, std::string const& name) : TypeInfoBase(typeID, name, Flag::IsClassStruct) {};

        std::vector<TypeInfoField const*> GetReflectedFields() const;
        bool HasArrayProperties() const;
    };

    //-------------------------------------------------------------------------

    struct EnumDataConstant
    {
        TypeID      ID;
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
