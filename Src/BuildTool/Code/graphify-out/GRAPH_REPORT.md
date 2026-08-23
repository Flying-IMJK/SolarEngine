# Graph Report - E:\EngineProject\SolarEngine\Src\BuildTool\Code  (2026-08-23)

## Corpus Check
- Corpus is ~47,790 words - fits in a single context window. You may not need a graph.

## Summary
- 1391 nodes · 3376 edges · 59 communities
- Extraction: 88% EXTRACTED · 12% INFERRED · 0% AMBIGUOUS · INFERRED: 420 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `56f04c61`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- C++ Bindings Generator
- C# Bindings Generator
- Clang Parser Toolchain
- Command Parser Library
- Core String Utilities
- C++ Type Generation
- Clang Cursor Utilities
- Parser Scope Tracking
- Reflection Project Model
- Parser Context State
- Type Database Storage
- Filesystem Utilities
- Mustache Renderer Core
- Mustache Data Model
- Core Type Utilities
- API Metadata Flags
- Native Type Stubs
- Property Metadata
- Mustache Formatting
- Binding Type Model
- Reflection Macro Options
- Field Type Resolution
- Visitor Error Logging
- String Identifier Core
- Mark Macro Storage
- Function API Metadata
- BuildTool CMake Setup
- Collection ABI Model
- Bindings Module Info
- Enum Metadata
- Type Database Cleanup
- Generator Templates
- TypeInfo Base Model
- Template Type Names
- Mark API Options
- Friendly Name Parsing
- Event Template Metadata
- Field Metadata
- Mustache Context Stack
- Event Metadata
- Field Template Metadata
- Parameter Metadata
- C++ Generator Core
- C++ Meta Generation
- Type Reference Model
- Mustache Context Core
- Template Instantiation
- Macro Visitor
- Generated Header Includes
- Resource Type Model
- Parameter Template Metadata
- C++ Enum Generation
- Injected Code Model
- Property Paths
- Managed Type Mapping
- Mustache Lambdas
- Type Generator Headers
- Bindings Module Generation

## God Nodes (most connected - your core abstractions)
1. `TypeInfoStruct` - 87 edges
2. `ClangParserContext` - 64 edges
3. `TypeDatabase` - 55 edges
4. `TypeInfoFunc` - 51 edges
5. `StripTypeQualifiers()` - 43 edges
6. `basic_data` - 42 edges
7. `TypeInfoStructTemplate` - 41 edges
8. `StringID` - 40 edges
9. `MarkMacro` - 35 edges
10. `PropertyData` - 34 edges

## Surprising Connections (you probably didn't know these)
- `FindTypeMappingForPod()` --calls--> `FindTypeMapping()`  [INFERRED]
  Clang/ClangParser.cpp → CodeGenerators/CodeGenerator_BindingsTypeMap.cpp
- `FindTypeMappingForPod()` --calls--> `StripTypeQualifiers()`  [INFERRED]
  Clang/ClangParser.cpp → CodeGenerators/CodeGenerator_BindingsTypeMap.cpp
- `DoesCppTypeMatch()` --calls--> `StripTypeQualifiers()`  [INFERRED]
  Clang/ClangParser.cpp → CodeGenerators/CodeGenerator_BindingsTypeMap.cpp
- `IsKnownNonPodValueType()` --calls--> `StripTypeQualifiers()`  [INFERRED]
  Clang/ClangParser.cpp → CodeGenerators/CodeGenerator_BindingsTypeMap.cpp
- `FindApiTypeByCppType()` --calls--> `StripTypeQualifiers()`  [INFERRED]
  Clang/ClangParser.cpp → CodeGenerators/CodeGenerator_BindingsTypeMap.cpp

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Platform Specific LLVM Linkage** — src_buildtool_code_cmakelists_windows_llvm_libraries, src_buildtool_code_cmakelists_linux_libclang, src_buildtool_code_cmakelists_macos_libclang [INFERRED 0.85]
- **BuildTool Source Groups** — src_buildtool_code_cmakelists_core_sources, src_buildtool_code_cmakelists_database_sources, src_buildtool_code_cmakelists_codegenerator_sources, src_buildtool_code_cmakelists_clang_sources [INFERRED 0.95]

## Communities (59 total, 0 thin omitted)

### Community 0 - "C++ Bindings Generator"
Cohesion: 0.08
Nodes (106): RegisterTypeNameAliases(), BindingsCppGenerator, BuildCallArgs, BuildForwardArgs, BuildWrapperParams, CanGenerateVariantFieldAccess, GenerateCollectionReturn, GenerateCppClass (+98 more)

### Community 1 - "C# Bindings Generator"
Cohesion: 0.06
Nodes (86): BindingsCSharpGenerator, BuildCSharpCallArgs, BuildCSharpInteropParams, BuildCSharpParams, Generate, GenerateBinaryModuleAssemblyInfo, GenerateCSharpAccessorProperty, GenerateCSharpClass (+78 more)

### Community 2 - "Clang Parser Toolchain"
Cohesion: 0.12
Nodes (48): CalculateStructureIsPod(), ClangParser, AddClangSystemInclude, AddMsvcToolchainArgs, AddUniquePath, ClangParser::ClangParser(), GetEnvironmentVariableValue, GetParentPath (+40 more)

### Community 3 - "Command Parser Library"
Cohesion: 0.08
Nodes (27): CmdBase, function, ostream, ArgumentCountChecker, ArgumentCountChecker<std::vector<T>>, Variadic, Variadic, CallbackArgs (+19 more)

### Community 4 - "Core String Utilities"
Cohesion: 0.07
Nodes (34): string_view, EnumFlags, m_value, uint32, vector, TopologicalSorter, Sort, VisitNode() (+26 more)

### Community 5 - "C++ Type Generation"
Cohesion: 0.11
Nodes (43): Generator, string, stringstream, TypeDatabase, vector, CppGenerateType(), GenerateAreAllPropertiesEqualMethod(), GenerateArrayAccessorMethod() (+35 more)

### Community 6 - "Clang Cursor Utilities"
Cohesion: 0.08
Nodes (35): AccessLevel, CXCursor, CXTranslationUnit, QualType, string, vector, GetAccessLevel(), GetAllBaseClasses() (+27 more)

### Community 7 - "Parser Scope Tracking"
Cohesion: 0.10
Nodes (40): AppendMacroMetadata(), CalculateFullNamespace(), CalculateFullStructScope(), GetNamespaces, GetStructScopes, PopNamespace, PopStruct, PushNamespace (+32 more)

### Community 8 - "Reflection Project Model"
Cohesion: 0.07
Nodes (33): vector, HeaderID, ProjectID, string, uint32, vector, HeaderInfo, checksum (+25 more)

### Community 9 - "Parser Context State"
Cohesion: 0.06
Nodes (32): ClangParserContext, AddTemplateType, AddTypeDef, CheckForOrphanedReflectionMacros, FindInjectCodeFormHead, GetAssemblyInfoForHeader, GetHeaderInfo, headersToVisit (+24 more)

### Community 10 - "Type Database Storage"
Cohesion: 0.12
Nodes (33): string, string, TypeInfoBase, unique_ptr, vector, TypeDatabase, BeginTransaction, Connect (+25 more)

### Community 11 - "Filesystem Utilities"
Cohesion: 0.12
Nodes (32): CreateDirectory(), DeleteDirectory(), DeleteFile(), DirectoryExists(), DirectoryGetFiles(), FileExists(), GetCurrentProcessDirectory(), GetCurrentProcessPath() (+24 more)

### Community 12 - "Mustache Renderer Core"
Cohesion: 0.14
Nodes (15): escape_handler, basic_mustache, error_message_, escape_, root_component_, component, children, position (+7 more)

### Community 13 - "Mustache Data Model"
Cohesion: 0.10
Nodes (16): basic_lambda, basic_lambda2, basic_list, basic_object, basic_partial, basic_data, lambda_, list_ (+8 more)

### Community 14 - "Core Type Utilities"
Cohesion: 0.14
Nodes (28): int32, string, string_view, TypeID, TypeIDCore, vector, Utils::CombineStringList(), Utils::GetCoreTypeID() (+20 more)

### Community 15 - "API Metadata Flags"
Cohesion: 0.07
Nodes (29): HeaderID, TypeID, TypeInfoStructTemplate, APIAttributes, APIIsAbstract, APIIsInterface, APIIsNativeInvokeUseName, APIIsSealed (+21 more)

### Community 16 - "Native Type Stubs"
Cohesion: 0.22
Nodes (26): AddAvailableFullType(), AddAvailableType(), AddEnumMemberFromDefault(), AddStubForCppType(), AddStubForCSharpType(), BindingsCSharpGenerator::GenerateNativeTypeStubs(), CollectFunctionStubs(), string (+18 more)

### Community 17 - "Property Metadata"
Cohesion: 0.08
Nodes (17): string_view, PropertyData, arraySize, category, description, flags, isDevOnly, isToolsReadOnly (+9 more)

### Community 18 - "Mustache Formatting"
Cohesion: 0.12
Nodes (16): delimiter_set, begin, default_begin, default_end, html_escape(), line_buffer_state, contained_section_tag, mstch_tag (+8 more)

### Community 19 - "Binding Type Model"
Cohesion: 0.08
Nodes (27): BindingTypeKind, BindingDiagnostic, filePath, lineNumber, message, BindingGenerationContext, diagnostics, inputHash (+19 more)

### Community 20 - "Reflection Macro Options"
Cohesion: 0.24
Nodes (22): FindMarkMacro, ApplyEventOptions(), ApplyFieldOptions(), ApplyStructOptions(), CXChildVisitResult, CXClientData, CXCursor, HeaderID (+14 more)

### Community 21 - "Field Type Resolution"
Cohesion: 0.12
Nodes (20): CXType, int32, string, TypeDatabase, TypeInfoBase, vector, FieldTypeInfo, isConstantArray (+12 more)

### Community 22 - "Visitor Error Logging"
Cohesion: 0.13
Nodes (14): Char, LogError(), CXChildVisitResult, CXClientData, CXCursor, HeaderID, VisitEnum(), VisitEnumContents() (+6 more)

### Community 23 - "String Identifier Core"
Cohesion: 0.12
Nodes (15): string, string_view, uint32, uint64, hash<SE::BuildTool::StringID>, StringID, Invalid, m_cache (+7 more)

### Community 24 - "Mark Macro Storage"
Cohesion: 0.10
Nodes (17): AddMarkMacro, HeaderID, MacroTypeEnum, unique_ptr, MarkMacro, api, fileColumn, fileEndColumn (+9 more)

### Community 25 - "Function API Metadata"
Cohesion: 0.10
Nodes (20): TypeInfoFuncTemplate, access, APIIsPropertie, APIIsSealed, APIIsStatic, APINoProxy, attributes, comment (+12 more)

### Community 26 - "BuildTool CMake Setup"
Cohesion: 0.12
Nodes (20): BuildTool CMake Configuration, Clang Sources, CodeGenerator Sources, Core Sources, C++17 Standard, Database Sources, fmt::fmt, Linux libclang.so.12 (+12 more)

### Community 27 - "Collection ABI Model"
Cohesion: 0.11
Nodes (16): CollectionAbiInfo, elementCppType, fixedElementCount, kind, CppTypeInfo, arraySize, baseType, genericArgs (+8 more)

### Community 28 - "Bindings Module Info"
Cohesion: 0.12
Nodes (18): BinaryModuleInfo, assemblyDir, assemblyType, headers, name, BindingsHeaderInfo, assemblyDir, assemblyName (+10 more)

### Community 29 - "Enum Metadata"
Cohesion: 0.14
Nodes (14): EnumDataConstant, description, ID, label, value, string, TypeIDCore, TypeInfoEnum (+6 more)

### Community 30 - "Type Database Cleanup"
Cohesion: 0.24
Nodes (17): HeaderID, ProjectID, TypeInfoBase, vector, DeleteObseleteHeadersAndTypes, DeleteObseleteProjects, DeleteTypesForHeader, GetAllTypes (+9 more)

### Community 31 - "Generator Templates"
Cohesion: 0.12
Nodes (15): Generator, m_CodeCppClassTemplate, m_CodeCppEnumTemplate, m_CodeCppMetaTemplate, m_CodeModuleTemplate, m_engineTypeRegistrationFile, m_errorMessage, m_moduleFile (+7 more)

### Community 32 - "TypeInfo Base Model"
Cohesion: 0.13
Nodes (15): TypeInfoBase, assemblyDir, assemblyName, comment, flag, headerID, isAPI, isDevOnly (+7 more)

### Community 33 - "Template Type Names"
Cohesion: 0.36
Nodes (15): CXType, int32, string, FindTopLevelChar(), GetFullTypeName(), GetQualifiedTemplateInstantiationName(), IsQualifiedTypeName(), IsTemplateParameter() (+7 more)

### Community 34 - "Mark API Options"
Cohesion: 0.14
Nodes (14): MarkAPI, attributes, inBuildMapType, IsAbstract, IsNativeInvokeUseName, IsNoConstructor, IsNoProxy, IsNoSpawn (+6 more)

### Community 35 - "Friendly Name Parsing"
Cohesion: 0.24
Nodes (11): string, GenerateFriendlyName(), GetFriendlyName, TryParseResourceRegistrationMacroString, GetCategory, GetFriendlyName, GetPropertyDescriptor, HasArrayProperties (+3 more)

### Community 36 - "Event Template Metadata"
Cohesion: 0.15
Nodes (13): AccessLevel, vector, TypeInfoEventTemplate, access, attributes, comment, cppType, isAPI (+5 more)

### Community 37 - "Field Metadata"
Cohesion: 0.15
Nodes (13): TypeInfoField, APIIsReadOnly, arraySize, attributes, comment, defaultValue, isAPI, isReflect (+5 more)

### Community 38 - "Mustache Context Stack"
Cohesion: 0.29
Nodes (6): context, items_, context_pusher, vector, split(), string_type

### Community 39 - "Event Metadata"
Cohesion: 0.17
Nodes (12): AccessLevel, TypeInfoEvent, access, attributes, comment, cppType, isAPI, isReflect (+4 more)

### Community 40 - "Field Template Metadata"
Cohesion: 0.17
Nodes (12): TypeInfoFieldTemplate, APIIsReadOnly, attributes, comment, defaultValue, isAPI, isReflect, isStatic (+4 more)

### Community 41 - "Parameter Metadata"
Cohesion: 0.17
Nodes (12): TypeInfoParam, arraySize, attributes, comment, defaultValue, isConst, isOut, isPointer (+4 more)

### Community 42 - "C++ Generator Core"
Cohesion: 0.33
Nodes (10): Generator, string, stringstream, DecodeInjectCodeString(), FindMatchingParen(), LoadTemplateFile, SaveStreamToFile, IsEscaped() (+2 more)

### Community 43 - "C++ Meta Generation"
Cohesion: 0.25
Nodes (9): Generator, string, stringstream, TypeDatabase, TypeInfoBase, CppGenerateMeta(), CppParseMeta(), GenerateFile() (+1 more)

### Community 44 - "Type Reference Model"
Cohesion: 0.18
Nodes (10): TypeRefTemplate, arraySize, genericArgs, isArray, isConst, isMoveRef, isPointer, isRef (+2 more)

### Community 45 - "Mustache Context Core"
Cohesion: 0.20
Nodes (8): basic_context, get, get_partial, pop, push, context_internal, delim_set, line_buffer

### Community 46 - "Template Instantiation"
Cohesion: 0.49
Nodes (9): TypeDefData, unique_ptr, vector, InstantiateEvent(), InstantiateField(), InstantiateFunction(), InstantiateParam(), InstantiateTemplateType() (+1 more)

### Community 47 - "Macro Visitor"
Cohesion: 0.22
Nodes (7): MacroTypeEnum, GetMarkMacroText, CXChildVisitResult, CXCursor, string, VisitMacro(), TypeDatabase

### Community 48 - "Generated Header Includes"
Cohesion: 0.25
Nodes (8): GenerateNativeTypeStubs, HeaderID, string_view, FindHeaderInProject(), AppendAPIIncludesIfNeeded, Generate, GenerateTypeInfoFileHeader, LogError()

### Community 49 - "Resource Type Model"
Cohesion: 0.22
Nodes (9): TypeID, ReflectedResourceType, className, friendlyName, headerID, namespaceName, parents, resourceTypeID (+1 more)

### Community 50 - "Parameter Template Metadata"
Cohesion: 0.22
Nodes (9): string, TypeInfoParamTemplate, attributes, comment, defaultValue, isOut, marshalAs, name (+1 more)

### Community 51 - "C++ Enum Generation"
Cohesion: 0.32
Nodes (6): Generator, string, stringstream, CppGenerateEnum(), GenerateFile(), Generator

### Community 52 - "Injected Code Model"
Cohesion: 0.25
Nodes (8): IsCSharpCode(), HeaderID, TypeInfoInjectedCode, code, headID, lang, lineNumber, InjectEnum

### Community 53 - "Property Paths"
Cohesion: 0.29
Nodes (4): vector, PropertyPath, m_elements, Element

### Community 54 - "Managed Type Mapping"
Cohesion: 0.29
Nodes (7): TypeMapping, cppType, csInterop, csType, isBlittable, isObject, isString

### Community 55 - "Mustache Lambdas"
Cohesion: 0.52
Nodes (3): type1_, type2_, basic_renderer

### Community 56 - "Type Generator Headers"
Cohesion: 0.33
Nodes (4): string, Generator, TypeDatabase, TypeInfoBase

### Community 57 - "Bindings Module Generation"
Cohesion: 0.47
Nodes (6): BuildBindingsHeaderInfoFromTypes(), TypeDatabase, TypeInfoBase, vector, EnsureUniqueAPIFunctionNames(), GenerateModuleCodeFile

## Knowledge Gaps
- **428 isolated node(s):** `m_context`, `m_totalParsingTime`, `m_totalVisitingTime`, `m_reflectionDataPath`, `tokens` (+423 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `ClangParserContext` connect `Parser Context State` to `Template Type Names`, `Clang Parser Toolchain`, `Clang Cursor Utilities`, `Parser Scope Tracking`, `Reflection Project Model`, `Template Instantiation`, `Macro Visitor`, `Reflection Macro Options`, `Field Type Resolution`, `Visitor Error Logging`, `Mark Macro Storage`?**
  _High betweenness centrality (0.154) - this node is a cross-community bridge._
- **Why does `TypeInfoStruct` connect `C++ Type Generation` to `C++ Bindings Generator`, `C# Bindings Generator`, `TypeInfo Base Model`, `Friendly Name Parsing`, `Field Metadata`, `Event Metadata`, `Template Instantiation`, `Reflection Macro Options`, `Property Paths`, `String Identifier Core`, `Bindings Module Generation`, `Bindings Module Info`, `Enum Metadata`?**
  _High betweenness centrality (0.153) - this node is a cross-community bridge._
- **Why does `StringID` connect `String Identifier Core` to `TypeInfo Base Model`, `Friendly Name Parsing`, `C++ Type Generation`, `Clang Cursor Utilities`, `Reflection Project Model`, `C++ Generator Core`, `Type Database Storage`, `Resource Type Model`, `Reflection Macro Options`, `Field Type Resolution`, `Property Paths`, `Enum Metadata`?**
  _High betweenness centrality (0.075) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `ClangParserContext` (e.g. with `VisitStructureContents()` and `VisitTemplateStructureContents()`) actually correct?**
  _`ClangParserContext` has 2 INFERRED edges - model-reasoned connections that need verification._
- **Are the 21 inferred relationships involving `StripTypeQualifiers()` (e.g. with `DoesCppTypeMatch()` and `FindApiTypeByCppType()`) actually correct?**
  _`StripTypeQualifiers()` has 21 INFERRED edges - model-reasoned connections that need verification._
- **What connects `m_context`, `m_totalParsingTime`, `m_totalVisitingTime` to the rest of the system?**
  _428 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `C++ Bindings Generator` be split into smaller, more focused modules?**
  _Cohesion score 0.07539616346955796 - nodes in this community are weakly interconnected._