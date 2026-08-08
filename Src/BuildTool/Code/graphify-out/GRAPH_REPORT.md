# Graph Report - E:\EngineProject\SolarEngine\Src\BuildTool\Code  (2026-08-08)

## Corpus Check
- Corpus is ~44,286 words - fits in a single context window. You may not need a graph.

## Summary
- 1284 nodes · 3026 edges · 49 communities (47 shown, 2 thin omitted)
- Extraction: 88% EXTRACTED · 12% INFERRED · 0% AMBIGUOUS · INFERRED: 356 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- CodeGenerator BindingsTypeMap.cpp / StripTypeQualifiers()
- ReflectionDatabase / StringID
- CodeGenerator CPP.cpp / Generate
- Parser / string
- CodeGenerator BindingsCpp.cpp / string
- BindingTypeInfo / CodeGenerator BindingsModel.cpp
- GenerateTypeInfoFile() / data
- BindingsHeaderInfo / CppTypeInfo
- ClangUtils.h / Dictionary
- ClangVisitors Structure.cpp / VisitTemplateStructure()
- FileSystem.h / string
- ClangParserContext.cpp / string
- ClangParserContext / .GenerateTypeID()
- ClangParserContext.h / VisitTranslationUnit()
- Utils.cpp / string
- ApiClass / access
- Reflector / Reflector.cpp
- mustache.hpp / .parse()
- MarkMacro / VisitStructure()
- CodeGenerator BindingsNativeStubs.cpp / string
- ApiProperty / attributes
- .render component() / context internal
- basic data / .render section()
- ClangParser / Milliseconds
- BindingInfo / assemblyDir
- PropertyData / .GetCategory()
- ApiFunction / access
- TypeData / Flags
- .basic data() / basic lambda t
- ProjectInfo / .GetProjectID()
- DataTypes.cpp / string
- ApiEnum / access
- component / .walk children()
- ApiEvent / string
- ApiField / arraySize
- DataTypes.h / EnumDataConstant
- HeaderInfo / string
- string type / context
- ApiParam / arraySize
- ApiInterface / AccessLevel
- string / vector
- EnumFlags / uint32
- SolutionInfo / ClangParser::ClangParser()
- FindMarkMacro / .MarkMacro()
- .IsFlag() / GetValueFromEnumLabel
- Sort / TopologicalSort.h
- basic context / .context internal()
- PlatformClock / .Now()

## God Nodes (most connected - your core abstractions)
1. `ClangParserContext` - 58 edges
2. `ReflectionDatabase` - 52 edges
3. `ApiFunction` - 42 edges
4. `basic_data` - 42 edges
5. `StripTypeQualifiers()` - 41 edges
6. `TypeData` - 40 edges
7. `StringID` - 39 edges
8. `ApiClass` - 37 edges
9. `PropertyData` - 36 edges
10. `MarkMacro` - 31 edges

## Surprising Connections (you probably didn't know these)
- `VisitMacro()` --calls--> `GetMarkMacroText()`  [INFERRED]
  Clang/ClangVisitors_Macro.cpp → Core/Utils.cpp
- `VisitMethod()` --calls--> `ArePropertyAccessorTypesCompatible()`  [INFERRED]
  Clang/ClangVisitors_Structure.cpp → CodeGenerators/CodeGenerator_BindingsTypeMap.cpp
- `ProcessHeaderFile` --calls--> `GetMarkMacroText()`  [INFERRED]
  Reflector.h → Core/Utils.cpp
- `Parse` --references--> `HeaderInfo`  [EXTRACTED]
  Clang/ClangParser.h → Database/ReflectionProjectTypes.h
- `ReflectRegisteredHeaders` --calls--> `Parse`  [INFERRED]
  Reflector.h → Clang/ClangParser.h

## Import Cycles
- None detected.

## Communities (49 total, 2 thin omitted)

### Community 0 - "CodeGenerator BindingsTypeMap.cpp / StripTypeQualifiers()"
Cohesion: 0.07
Nodes (109): GetVariantToNativeConvert, BindingsCSharpGenerator, BuildCSharpCallArgs, BuildCSharpInteropParams, BuildCSharpParams, Generate, GenerateAll, GenerateBinaryModuleAssemblyInfo (+101 more)

### Community 1 - "ReflectionDatabase / StringID"
Cohesion: 0.06
Nodes (72): string, string_view, uint32, uint64, hash<SE::BuildTool::StringID>, StringID, Invalid, m_cache (+64 more)

### Community 2 - "CodeGenerator CPP.cpp / Generate"
Cohesion: 0.06
Nodes (70): GenerateNativeTypeStubs, ClearApiTypeNameAliases(), BuildBindingsHeaderInfoFromTypes(), CalculateStructureIsPod(), CollectApiInjectedCode(), ApiClass, ApiInjectedCode, Generator (+62 more)

### Community 3 - "Parser / string"
Cohesion: 0.08
Nodes (27): CmdBase, function, ostream, ArgumentCountChecker, ArgumentCountChecker<std::vector<T>>, Variadic, Variadic, CallbackArgs (+19 more)

### Community 4 - "CodeGenerator BindingsCpp.cpp / string"
Cohesion: 0.14
Nodes (46): BindingsCppGenerator, BuildCallArgs, BuildForwardArgs, BuildWrapperParams, GenerateCollectionReturn, GenerateCppClass, GenerateCppEnum, GenerateCppEventWrappers (+38 more)

### Community 5 - "BindingTypeInfo / CodeGenerator BindingsModel.cpp"
Cohesion: 0.05
Nodes (44): BindingTypeKind, AddBindingDiagnostic(), BindingCallable, invocation, BindingDiagnostic, filePath, lineNumber, message (+36 more)

### Community 6 - "GenerateTypeInfoFile() / data"
Cohesion: 0.11
Nodes (39): Generator, string, stringstream, TypeData, CppGenerateEnum(), GenerateFile(), Generator, Generator (+31 more)

### Community 7 - "BindingsHeaderInfo / CppTypeInfo"
Cohesion: 0.05
Nodes (39): ApiInjectedCode, code, lang, lineNumber, BinaryModuleInfo, assemblyDir, assemblyType, headers (+31 more)

### Community 8 - "ClangUtils.h / Dictionary"
Cohesion: 0.08
Nodes (35): AccessLevel, CXCursor, CXTranslationUnit, QualType, string, vector, GetAccessLevel(), GetAllBaseClasses() (+27 more)

### Community 9 - "ClangVisitors Structure.cpp / VisitTemplateStructure()"
Cohesion: 0.10
Nodes (34): TryGetValue, ApplyMemberMacroMetadata(), ApiParam, CXChildVisitResult, CXClientData, CXCursor, CXType, int32 (+26 more)

### Community 10 - "FileSystem.h / string"
Cohesion: 0.12
Nodes (32): CreateDirectory(), DeleteDirectory(), DeleteFile(), DirectoryExists(), DirectoryGetFiles(), FileExists(), GetCurrentProcessDirectory(), GetCurrentProcessPath() (+24 more)

### Community 11 - "ClangParserContext.cpp / string"
Cohesion: 0.17
Nodes (32): AppendMacroMetadata(), CalculateFullNamespace(), CalculateFullStructScope(), AddTemplateType, GetAssemblyInfoForHeader, GetNamespaces, GetStructScopes, PopNamespace (+24 more)

### Community 12 - "ClangParserContext / .GenerateTypeID()"
Cohesion: 0.07
Nodes (28): ClangParserContext, AddTypeDef, CheckForOrphanedReflectionMacros, headersToVisit, m_currentNamespace, m_currentStructScope, m_errorMessage, m_MarkMacros (+20 more)

### Community 13 - "ClangParserContext.h / VisitTranslationUnit()"
Cohesion: 0.10
Nodes (22): Char, LogError(), CXChildVisitResult, CXClientData, CXCursor, HeaderID, VisitEnum(), VisitEnumContents() (+14 more)

### Community 14 - "Utils.cpp / string"
Cohesion: 0.14
Nodes (28): int32, string, string_view, TypeID, TypeIDCore, vector, Utils::CombineStringList(), Utils::GetCoreTypeID() (+20 more)

### Community 15 - "ApiClass / access"
Cohesion: 0.07
Nodes (29): ApiClass, access, attributes, baseClassName, comment, events, fields, functions (+21 more)

### Community 16 - "Reflector / Reflector.cpp"
Cohesion: 0.13
Nodes (22): HeaderProcessResult, HeaderTimestamp, ProjectObject, string, vector, ReflectionDatabase, string, vector (+14 more)

### Community 17 - "mustache.hpp / .parse()"
Cohesion: 0.12
Nodes (16): delimiter_set, begin, default_begin, default_end, html_escape(), line_buffer_state, contained_section_tag, mstch_tag (+8 more)

### Community 18 - "MarkMacro / VisitStructure()"
Cohesion: 0.09
Nodes (22): AddMarkMacro, SetModuleClassName, string_view, HeaderID, ReflectionMacroType, MarkMacro, fileColumn, fileEndColumn (+14 more)

### Community 19 - "CodeGenerator BindingsNativeStubs.cpp / string"
Cohesion: 0.23
Nodes (25): AddAvailableType(), AddEnumMemberFromDefault(), AddStubForCppType(), AddStubForCSharpType(), BindingsCSharpGenerator::GenerateNativeTypeStubs(), CollectFunctionStubs(), ApiParam, string (+17 more)

### Community 20 - "ApiProperty / attributes"
Cohesion: 0.09
Nodes (22): ApiProperty, attributes, comment, cppType, getterAccess, getterCppType, getterEntryPoint, getterName (+14 more)

### Community 21 - ".render component() / context internal"
Cohesion: 0.23
Nodes (10): escape_handler, basic_mustache, error_message_, escape_, root_component_, context_internal, delim_set, line_buffer (+2 more)

### Community 22 - "basic data / .render section()"
Cohesion: 0.13
Nodes (8): basic_data, lambda_, list_, obj_, partial_, str_, type_, end

### Community 23 - "ClangParser / Milliseconds"
Cohesion: 0.14
Nodes (13): ClangParser, m_context, m_reflectionDataPath, m_totalParsingTime, m_totalVisitingTime, Parse, vector, Milliseconds (+5 more)

### Community 24 - "BindingInfo / assemblyDir"
Cohesion: 0.10
Nodes (21): BindingInfo, assemblyDir, assemblyName, attributes, baseClassName, bindingProperties, comment, events (+13 more)

### Community 25 - "PropertyData / .GetCategory()"
Cohesion: 0.10
Nodes (17): string_view, PropertyData, arraySize, category, description, flags, isDevOnly, isToolsReadOnly (+9 more)

### Community 26 - "ApiFunction / access"
Cohesion: 0.10
Nodes (20): ApiFunction, access, attributes, comment, entryPoint, isConst, isDeprecated, isHidden (+12 more)

### Community 27 - "TypeData / Flags"
Cohesion: 0.11
Nodes (17): TypeIDCore, TypeData, bindingInfo, enumConstants, flags, headerID, isAPI, isDevOnly (+9 more)

### Community 28 - ".basic data() / basic lambda t"
Cohesion: 0.18
Nodes (11): basic_lambda, basic_lambda2, basic_list, basic_object, basic_partial, basic_lambda_t, type1_, type2_ (+3 more)

### Community 29 - "ProjectInfo / .GetProjectID()"
Cohesion: 0.12
Nodes (17): ProjectID, uint32, ProjectInfo, dependencies, dependencyCount, dirtyHeaders, exportMacro, headerFiles (+9 more)

### Community 30 - "DataTypes.cpp / string"
Cohesion: 0.21
Nodes (13): string, GenerateFriendlyName(), GetFriendlyName, TryParseResourceRegistrationMacroString, AddEnumConstant, GetCategory, GetFriendlyName, GetPropertyDescriptor (+5 more)

### Community 31 - "ApiEnum / access"
Cohesion: 0.14
Nodes (14): ApiEnum, access, attributes, comment, isDeprecated, lineNumber, name, namespaceScopeList (+6 more)

### Community 32 - "component / .walk children()"
Cohesion: 0.21
Nodes (8): component, children, position, tag, text, string_size_type, walk_callback, walk_control

### Community 33 - "ApiEvent / string"
Cohesion: 0.15
Nodes (11): ApiEvent, access, attributes, comment, cppType, isStatic, lineNumber, name (+3 more)

### Community 34 - "ApiField / arraySize"
Cohesion: 0.15
Nodes (13): ApiField, arraySize, attributes, comment, cppType, defaultValue, isDeprecated, isHidden (+5 more)

### Community 35 - "DataTypes.h / EnumDataConstant"
Cohesion: 0.15
Nodes (9): EnumDataConstant, description, ID, label, value, vector, PropertyPath, m_elements (+1 more)

### Community 36 - "HeaderInfo / string"
Cohesion: 0.22
Nodes (9): HeaderID, string, HeaderInfo, checksum, fileContents, filePath, headerId, projectID (+1 more)

### Community 37 - "string type / context"
Cohesion: 0.29
Nodes (6): context, items_, context_pusher, vector, split(), string_type

### Community 38 - "ApiParam / arraySize"
Cohesion: 0.17
Nodes (12): ApiParam, arraySize, attributes, comment, cppType, defaultValue, isConst, isOut (+4 more)

### Community 39 - "ApiInterface / AccessLevel"
Cohesion: 0.18
Nodes (11): ApiInterface, access, attributes, comment, functions, isDeprecated, lineNumber, name (+3 more)

### Community 40 - "string / vector"
Cohesion: 0.20
Nodes (6): string, string_view, vector, Generator, ReflectionDatabase, TypeData

### Community 41 - "EnumFlags / uint32"
Cohesion: 0.33
Nodes (4): EnumFlags, m_value, uint32, Enum

### Community 42 - "SolutionInfo / ClangParser::ClangParser()"
Cohesion: 0.25
Nodes (8): ClangParser::ClangParser(), ReflectionDatabase, string, vector, SolutionInfo, excludedProjects, path, projects

### Community 43 - "FindMarkMacro / .MarkMacro()"
Cohesion: 0.36
Nodes (7): FindMarkMacro, FindReflectionMacroForMeta, GetHeaderInfo, CXCursor, HeaderID, ReflectionMacroType, CXSourceRange

### Community 45 - "Sort / TopologicalSort.h"
Cohesion: 0.48
Nodes (5): vector, TopologicalSorter, Sort, VisitNode(), Node

### Community 46 - "basic context / .context internal()"
Cohesion: 0.29
Nodes (5): basic_context, get, get_partial, pop, push

## Knowledge Gaps
- **383 isolated node(s):** `m_context`, `m_totalParsingTime`, `m_totalVisitingTime`, `m_reflectionDataPath`, `headerID` (+378 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `ClangParserContext` connect `ClangParserContext / .GenerateTypeID()` to `string / vector`, `ClangUtils.h / Dictionary`, `SolutionInfo / ClangParser::ClangParser()`, `FindMarkMacro / .MarkMacro()`, `ClangParserContext.cpp / string`, `ClangParserContext.h / VisitTranslationUnit()`, `ClangVisitors Structure.cpp / VisitTemplateStructure()`, `MarkMacro / VisitStructure()`, `ClangParser / Milliseconds`?**
  _High betweenness centrality (0.116) - this node is a cross-community bridge._
- **Why does `StringID` connect `ReflectionDatabase / StringID` to `ApiEvent / string`, `CodeGenerator CPP.cpp / Generate`, `DataTypes.h / EnumDataConstant`, `HeaderInfo / string`, `ClangUtils.h / Dictionary`, `ClangVisitors Structure.cpp / VisitTemplateStructure()`, `string / vector`, `.IsFlag() / GetValueFromEnumLabel`, `TypeData / Flags`, `ProjectInfo / .GetProjectID()`, `DataTypes.cpp / string`?**
  _High betweenness centrality (0.112) - this node is a cross-community bridge._
- **Why does `ApiFunction` connect `ApiFunction / access` to `CodeGenerator BindingsTypeMap.cpp / StripTypeQualifiers()`, `ApiEvent / string`, `DataTypes.h / EnumDataConstant`, `CodeGenerator BindingsCpp.cpp / string`, `BindingTypeInfo / CodeGenerator BindingsModel.cpp`, `ApiParam / arraySize`, `ApiInterface / AccessLevel`, `ClangParserContext.cpp / string`, `ApiClass / access`, `CodeGenerator BindingsNativeStubs.cpp / string`, `BindingInfo / assemblyDir`?**
  _High betweenness centrality (0.080) - this node is a cross-community bridge._
- **Are the 19 inferred relationships involving `StripTypeQualifiers()` (e.g. with `GetManagedToNativeConvert` and `GetNativeToManagedConvert`) actually correct?**
  _`StripTypeQualifiers()` has 19 INFERRED edges - model-reasoned connections that need verification._
- **What connects `m_context`, `m_totalParsingTime`, `m_totalVisitingTime` to the rest of the system?**
  _383 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `CodeGenerator BindingsTypeMap.cpp / StripTypeQualifiers()` be split into smaller, more focused modules?**
  _Cohesion score 0.06986492780624126 - nodes in this community are weakly interconnected._
- **Should `ReflectionDatabase / StringID` be split into smaller, more focused modules?**
  _Cohesion score 0.05823293172690763 - nodes in this community are weakly interconnected._