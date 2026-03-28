
add_subdirectory(Property)
add_subdirectory(MetaData)

CORE_ADD(
        CoreTypeConversions.h
        CoreTypeConversions.cpp
        CoreTypes.h
        CoreTypes.cpp
        DefaultTypeInfos.h

        IType.h
        IType.cpp
        TypeDescriptors.h
        TypeDescriptors.cpp
        TypeID.h
        TypeID.cpp
        TypeIDCore.h
        Types.h
        Types.cpp

        Info/TypeInfo.h
        Info/TypeCompositeInfo.cpp
        Info/TypeCompositeInfo.h
        Info/TypeEnumInfo.h
        Info/TypeEnumInfo.cpp
)