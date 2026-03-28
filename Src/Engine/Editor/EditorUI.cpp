#include "EditorUI.h"

#include "Core/Resource/ResourceSystem.h"
#include "Core/TypeSystem/Types.h"
// #include "Base/ThirdParty/implot/implot.h"
#include "Core/Logging/LoggingSystem.h"
#include "Core/Profiling.h"

#include "Runtime/UpdateContext.h"
#include "Runtime/Render/HeightLevelRenderer.h"
#include "Runtime/SGUI/ToolsUI/EngineDebugUI.h"
#include "Runtime/SGUI/GUICache.h"

#include "Editor/Core/EditorEmbeddedResources.inl"
#include "Editor/Core/EditorWindow.h"
#include "Editor/EditorWindow/ConsoleWindow.h"
#include "SGUI/FileBrowser/FileBrowserWindow.h"
#include "EditorWindow/ResourceSystemWindow.h"
// #include "Editor/EditorWindow/ResourceImporter.h"
// #include "EngineTools/Entity/ResourceEditors/ResourceEditor_MapEditor.h"
// #include "EngineTools/Entity/Tools/EditorTool_GamePreviewer.h"
// #include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
// #include "EngineTools/Core/ToolsEmbeddedResources.inl"

// #include "Engine/Entity/EntityWorld.h"
// #include "Engine/Entity/EntityWorldManager.h"
// #include "Engine/Entity/EntityWorldUpdateContext.h"
// #include "Engine/Render/DebugViews/DebugView_Render.h"





//-------------------------------------------------------------------------

//static uint8_t const g_encodedDataGreen[3328] = { 105, 86, 66, 79, 82, 119, 48, 75, 71, 103, 111, 65, 65, 65, 65, 78, 83, 85, 104, 69, 85, 103, 65, 65, 65, 66, 103, 65, 65, 65, 65, 89, 67, 65, 89, 65, 65, 65, 68, 103, 100, 122, 51, 52, 65, 65, 65, 65, 67, 88, 66, 73, 87, 88, 77, 65, 65, 65, 115, 84, 65, 65, 65, 76, 69, 119, 69, 65, 109, 112, 119, 89, 65, 65, 65, 70, 57, 50, 108, 85, 87, 72, 82, 89, 84, 85, 119, 54, 89, 50, 57, 116, 76, 109, 70, 107, 98, 50, 74, 108, 76, 110, 104, 116, 99, 65, 65, 65, 65, 65, 65, 65, 80, 68, 57, 52, 99, 71, 70, 106, 97, 50, 86, 48, 73, 71, 74, 108, 90, 50, 108, 117, 80, 83, 76, 118, 117, 55, 56, 105, 73, 71, 108, 107, 80, 83, 74, 88, 78, 85, 48, 119, 84, 88, 66, 68, 90, 87, 104, 112, 83, 72, 112, 121, 90, 86, 78, 54, 84, 108, 82, 106, 101, 109, 116, 106, 79, 87, 81, 105, 80, 122, 52, 103, 80, 72, 103, 54, 101, 71, 49, 119, 98, 87, 86, 48, 89, 83, 66, 52, 98, 87, 120, 117, 99, 122, 112, 52, 80, 83, 74, 104, 90, 71, 57, 105, 90, 84, 112, 117, 99, 122, 112, 116, 90, 88, 82, 104, 76, 121, 73, 103, 101, 68, 112, 52, 98, 88, 66, 48, 97, 122, 48, 105, 81, 87, 82, 118, 89, 109, 85, 103, 87, 69, 49, 81, 73, 69, 78, 118, 99, 109, 85, 103, 78, 105, 52, 119, 76, 87, 77, 119, 77, 68, 89, 103, 78, 122, 107, 117, 77, 84, 89, 48, 78, 106, 81, 52, 76, 67, 65, 121, 77, 68, 73, 120, 76, 122, 65, 120, 76, 122, 69, 121, 76, 84, 69, 49, 79, 106, 85, 121, 79, 106, 73, 53, 73, 67, 65, 103, 73, 67, 65, 103, 73, 67, 65, 105, 80, 105, 65, 56, 99, 109, 82, 109, 79, 108, 74, 69, 82, 105, 66, 52, 98, 87, 120, 117, 99, 122, 112, 121, 90, 71, 89, 57, 73, 109, 104, 48, 100, 72, 65, 54, 76, 121, 57, 51, 100, 51, 99, 117, 100, 122, 77, 117, 98, 51, 74, 110, 76, 122, 69, 53, 79, 84, 107, 118, 77, 68, 73, 118, 77, 106, 73, 116, 99, 109, 82, 109, 76, 88, 78, 53, 98, 110, 82, 104, 101, 67, 49, 117, 99, 121, 77, 105, 80, 105, 65, 56, 99, 109, 82, 109, 79, 107, 82, 108, 99, 50, 78, 121, 97, 88, 66, 48, 97, 87, 57, 117, 73, 72, 74, 107, 90, 106, 112, 104, 89, 109, 57, 49, 100, 68, 48, 105, 73, 105, 66, 52, 98, 87, 120, 117, 99, 122, 112, 52, 98, 88, 65, 57, 73, 109, 104, 48, 100, 72, 65, 54, 76, 121, 57, 117, 99, 121, 53, 104, 90, 71, 57, 105, 90, 83, 53, 106, 98, 50, 48, 118, 101, 71, 70, 119, 76, 122, 69, 117, 77, 67, 56, 105, 73, 72, 104, 116, 98, 71, 53, 122, 79, 110, 104, 116, 99, 69, 49, 78, 80, 83, 74, 111, 100, 72, 82, 119, 79, 105, 56, 118, 98, 110, 77, 117, 89, 87, 82, 118, 89, 109, 85, 117, 89, 50, 57, 116, 76, 51, 104, 104, 99, 67, 56, 120, 76, 106, 65, 118, 98, 87, 48, 118, 73, 105, 66, 52, 98, 87, 120, 117, 99, 122, 112, 122, 100, 69, 86, 50, 100, 68, 48, 105, 97, 72, 82, 48, 99, 68, 111, 118, 76, 50, 53, 122, 76, 109, 70, 107, 98, 50, 74, 108, 76, 109, 78, 118, 98, 83, 57, 52, 89, 88, 65, 118, 77, 83, 52, 119, 76, 51, 78, 85, 101, 88, 66, 108, 76, 49, 74, 108, 99, 50, 57, 49, 99, 109, 78, 108, 82, 88, 90, 108, 98, 110, 81, 106, 73, 105, 66, 52, 98, 87, 120, 117, 99, 122, 112, 107, 89, 122, 48, 105, 97, 72, 82, 48, 99, 68, 111, 118, 76, 51, 66, 49, 99, 109, 119, 117, 98, 51, 74, 110, 76, 50, 82, 106, 76, 50, 86, 115, 90, 87, 49, 108, 98, 110, 82, 122, 76, 122, 69, 117, 77, 83, 56, 105, 73, 72, 104, 116, 98, 71, 53, 122, 79, 110, 66, 111, 98, 51, 82, 118, 99, 50, 104, 118, 99, 68, 48, 105, 97, 72, 82, 48, 99, 68, 111, 118, 76, 50, 53, 122, 76, 109, 70, 107, 98, 50, 74, 108, 76, 109, 78, 118, 98, 83, 57, 119, 97, 71, 57, 48, 98, 51, 78, 111, 98, 51, 65, 118, 77, 83, 52, 119, 76, 121, 73, 103, 101, 71, 49, 119, 79, 107, 78, 121, 90, 87, 70, 48, 98, 51, 74, 85, 98, 50, 57, 115, 80, 83, 74, 66, 90, 71, 57, 105, 90, 83, 66, 81, 97, 71, 57, 48, 98, 51, 78, 111, 98, 51, 65, 103, 77, 106, 73, 117, 77, 105, 65, 111, 84, 87, 70, 106, 97, 87, 53, 48, 98, 51, 78, 111, 75, 83, 73, 103, 101, 71, 49, 119, 79, 107, 78, 121, 90, 87, 70, 48, 90, 85, 82, 104, 100, 71, 85, 57, 73, 106, 73, 119, 77, 106, 73, 116, 77, 68, 103, 116, 77, 84, 100, 85, 77, 106, 65, 54, 78, 68, 69, 54, 77, 84, 103, 114, 77, 68, 81, 54, 77, 68, 65, 105, 73, 72, 104, 116, 99, 68, 112, 78, 90, 88, 82, 104, 90, 71, 70, 48, 89, 85, 82, 104, 100, 71, 85, 57, 73, 106, 73, 119, 77, 106, 73, 116, 77, 68, 103, 116, 77, 84, 100, 85, 77, 106, 65, 54, 78, 68, 69, 54, 77, 84, 103, 114, 77, 68, 81, 54, 77, 68, 65, 105, 73, 72, 104, 116, 99, 68, 112, 78, 98, 50, 82, 112, 90, 110, 108, 69, 89, 88, 82, 108, 80, 83, 73, 121, 77, 68, 73, 121, 76, 84, 65, 52, 76, 84, 69, 51, 86, 68, 73, 119, 79, 106, 81, 120, 79, 106, 69, 52, 75, 122, 65, 48, 79, 106, 65, 119, 73, 105, 66, 52, 98, 88, 66, 78, 84, 84, 112, 74, 98, 110, 78, 48, 89, 87, 53, 106, 90, 85, 108, 69, 80, 83, 74, 52, 98, 88, 65, 117, 97, 87, 108, 107, 79, 106, 108, 108, 79, 68, 86, 107, 89, 106, 107, 52, 76, 87, 78, 105, 79, 68, 73, 116, 78, 68, 107, 122, 78, 105, 48, 52, 89, 109, 74, 104, 76, 87, 86, 107, 79, 68, 73, 50, 89, 109, 69, 49, 77, 106, 73, 120, 77, 83, 73, 103, 101, 71, 49, 119, 84, 85, 48, 54, 82, 71, 57, 106, 100, 87, 49, 108, 98, 110, 82, 74, 82, 68, 48, 105, 89, 87, 82, 118, 89, 109, 85, 54, 90, 71, 57, 106, 97, 87, 81, 54, 99, 71, 104, 118, 100, 71, 57, 122, 97, 71, 57, 119, 79, 106, 86, 107, 79, 84, 89, 51, 90, 84, 66, 106, 76, 84, 82, 108, 78, 122, 73, 116, 79, 87, 73, 48, 89, 83, 48, 53, 89, 106, 70, 108, 76, 87, 81, 120, 78, 84, 81, 49, 77, 106, 78, 108, 89, 87, 69, 119, 78, 121, 73, 103, 101, 71, 49, 119, 84, 85, 48, 54, 84, 51, 74, 112, 90, 50, 108, 117, 89, 87, 120, 69, 98, 50, 78, 49, 98, 87, 86, 117, 100, 69, 108, 69, 80, 83, 74, 52, 98, 88, 65, 117, 90, 71, 108, 107, 79, 106, 90, 104, 79, 71, 78, 105, 77, 106, 89, 119, 76, 84, 77, 120, 89, 109, 89, 116, 78, 68, 74, 109, 89, 83, 49, 104, 78, 68, 89, 48, 76, 84, 100, 105, 77, 87, 90, 105, 90, 84, 69, 50, 78, 71, 85, 50, 89, 83, 73, 103, 90, 71, 77, 54, 90, 109, 57, 121, 98, 87, 70, 48, 80, 83, 74, 112, 98, 87, 70, 110, 90, 83, 57, 119, 98, 109, 99, 105, 73, 72, 66, 111, 98, 51, 82, 118, 99, 50, 104, 118, 99, 68, 112, 68, 98, 50, 120, 118, 99, 107, 49, 118, 90, 71, 85, 57, 73, 106, 77, 105, 73, 72, 66, 111, 98, 51, 82, 118, 99, 50, 104, 118, 99, 68, 112, 74, 81, 48, 78, 81, 99, 109, 57, 109, 97, 87, 120, 108, 80, 83, 74, 122, 85, 107, 100, 67, 73, 69, 108, 70, 81, 122, 89, 120, 79, 84, 89, 50, 76, 84, 73, 117, 77, 83, 73, 43, 73, 68, 120, 52, 98, 88, 66, 78, 84, 84, 112, 73, 97, 88, 78, 48, 98, 51, 74, 53, 80, 105, 65, 56, 99, 109, 82, 109, 79, 108, 78, 108, 99, 84, 52, 103, 80, 72, 74, 107, 90, 106, 112, 115, 97, 83, 66, 122, 100, 69, 86, 50, 100, 68, 112, 104, 89, 51, 82, 112, 98, 50, 52, 57, 73, 109, 78, 121, 90, 87, 70, 48, 90, 87, 81, 105, 73, 72, 78, 48, 82, 88, 90, 48, 79, 109, 108, 117, 99, 51, 82, 104, 98, 109, 78, 108, 83, 85, 81, 57, 73, 110, 104, 116, 99, 67, 53, 112, 97, 87, 81, 54, 78, 109, 69, 52, 89, 50, 73, 121, 78, 106, 65, 116, 77, 122, 70, 105, 90, 105, 48, 48, 77, 109, 90, 104, 76, 87, 69, 48, 78, 106, 81, 116, 78, 50, 73, 120, 90, 109, 74, 108, 77, 84, 89, 48, 90, 84, 90, 104, 73, 105, 66, 122, 100, 69, 86, 50, 100, 68, 112, 51, 97, 71, 86, 117, 80, 83, 73, 121, 77, 68, 73, 121, 76, 84, 65, 52, 76, 84, 69, 51, 86, 68, 73, 119, 79, 106, 81, 120, 79, 106, 69, 52, 75, 122, 65, 48, 79, 106, 65, 119, 73, 105, 66, 122, 100, 69, 86, 50, 100, 68, 112, 122, 98, 50, 90, 48, 100, 50, 70, 121, 90, 85, 70, 110, 90, 87, 53, 48, 80, 83, 74, 66, 90, 71, 57, 105, 90, 83, 66, 81, 97, 71, 57, 48, 98, 51, 78, 111, 98, 51, 65, 103, 77, 106, 73, 117, 77, 105, 65, 111, 84, 87, 70, 106, 97, 87, 53, 48, 98, 51, 78, 111, 75, 83, 73, 118, 80, 105, 65, 56, 99, 109, 82, 109, 79, 109, 120, 112, 73, 72, 78, 48, 82, 88, 90, 48, 79, 109, 70, 106, 100, 71, 108, 118, 98, 106, 48, 105, 99, 50, 70, 50, 90, 87, 81, 105, 73, 72, 78, 48, 82, 88, 90, 48, 79, 109, 108, 117, 99, 51, 82, 104, 98, 109, 78, 108, 83, 85, 81, 57, 73, 110, 104, 116, 99, 67, 53, 112, 97, 87, 81, 54, 79, 87, 85, 52, 78, 87, 82, 105, 79, 84, 103, 116, 89, 50, 73, 52, 77, 105, 48, 48, 79, 84, 77, 50, 76, 84, 104, 105, 89, 109, 69, 116, 90, 87, 81, 52, 77, 106, 90, 105, 89, 84, 85, 121, 77, 106, 69, 120, 73, 105, 66, 122, 100, 69, 86, 50, 100, 68, 112, 51, 97, 71, 86, 117, 80, 83, 73, 121, 77, 68, 73, 121, 76, 84, 65, 52, 76, 84, 69, 51, 86, 68, 73, 119, 79, 106, 81, 120, 79, 106, 69, 52, 75, 122, 65, 48, 79, 106, 65, 119, 73, 105, 66, 122, 100, 69, 86, 50, 100, 68, 112, 122, 98, 50, 90, 48, 100, 50, 70, 121, 90, 85, 70, 110, 90, 87, 53, 48, 80, 83, 74, 66, 90, 71, 57, 105, 90, 83, 66, 81, 97, 71, 57, 48, 98, 51, 78, 111, 98, 51, 65, 103, 77, 106, 73, 117, 77, 105, 65, 111, 84, 87, 70, 106, 97, 87, 53, 48, 98, 51, 78, 111, 75, 83, 73, 103, 99, 51, 82, 70, 100, 110, 81, 54, 89, 50, 104, 104, 98, 109, 100, 108, 90, 68, 48, 105, 76, 121, 73, 118, 80, 105, 65, 56, 76, 51, 74, 107, 90, 106, 112, 84, 90, 88, 69, 43, 73, 68, 119, 118, 101, 71, 49, 119, 84, 85, 48, 54, 83, 71, 108, 122, 100, 71, 57, 121, 101, 84, 52, 103, 80, 67, 57, 121, 90, 71, 89, 54, 82, 71, 86, 122, 89, 51, 74, 112, 99, 72, 82, 112, 98, 50, 52, 43, 73, 68, 119, 118, 99, 109, 82, 109, 79, 108, 74, 69, 82, 106, 52, 103, 80, 67, 57, 52, 79, 110, 104, 116, 99, 71, 49, 108, 100, 71, 69, 43, 73, 68, 119, 47, 101, 72, 66, 104, 89, 50, 116, 108, 100, 67, 66, 108, 98, 109, 81, 57, 73, 110, 73, 105, 80, 122, 52, 98, 79, 66, 120, 77, 65, 65, 65, 68, 98, 85, 108, 69, 81, 86, 82, 73, 105, 90, 87, 86, 84, 50, 103, 100, 86, 82, 84, 71, 102, 50, 100, 109, 55, 118, 120, 74, 106, 78, 66, 87, 55, 67, 117, 86, 78, 111, 74, 81, 112, 89, 107, 114, 109, 121, 103, 105, 113, 70, 103, 82, 107, 101, 53, 99, 117, 108, 70, 81, 85, 72, 84, 104, 83, 105, 121, 83, 81, 109, 107, 87, 49, 105, 103, 73, 88, 98, 103, 82, 86, 69, 81, 88, 52, 107, 111, 88, 98, 107, 84, 56, 65, 48, 49, 81, 113, 72, 81, 82, 68, 87, 74, 68, 98, 80, 113, 47, 50, 76, 84, 74, 122, 74, 115, 55, 57, 56, 49, 49, 77, 87, 56, 121, 77, 43, 43, 78, 48, 82, 119, 89, 55, 116, 121, 90, 101, 55, 47, 118, 110, 68, 80, 102, 78, 49, 102, 89, 75, 105, 55, 117, 55, 104, 67, 80, 51, 69, 99, 83, 106, 116, 79, 78, 57, 112, 79, 69, 43, 43, 105, 71, 101, 43, 103, 71, 79, 48, 110, 68, 88, 97, 84, 43, 84, 116, 73, 103, 81, 80, 115, 106, 97, 66, 43, 48, 103, 107, 120, 66, 112, 104, 74, 54, 110, 113, 98, 110, 88, 80, 71, 71, 81, 67, 50, 84, 97, 80, 56, 85, 99, 84, 82, 66, 73, 106, 117, 75, 104, 119, 75, 50, 80, 57, 98, 110, 49, 103, 69, 114, 119, 120, 100, 69, 87, 67, 75, 115, 110, 66, 107, 109, 103, 67, 101, 119, 56, 115, 106, 109, 52, 116, 121, 66, 88, 65, 113, 119, 118, 68, 47, 102, 66, 67, 117, 84, 71, 107, 106, 65, 108, 111, 107, 119, 55, 55, 81, 81, 84, 70, 99, 65, 84, 114, 88, 82, 49, 108, 90, 89, 54, 84, 43, 84, 74, 114, 104, 116, 103, 65, 77, 115, 116, 70, 85, 119, 51, 100, 104, 111, 54, 120, 85, 52, 77, 86, 97, 117, 89, 79, 85, 113, 86, 113, 54, 66, 114, 71, 69, 108, 66, 84, 70, 89, 87, 81, 100, 82, 119, 67, 105, 73, 119, 115, 111, 111, 56, 70, 79, 84, 119, 76, 73, 76, 75, 43, 80, 69, 48, 81, 51, 79, 51, 82, 50, 119, 117, 105, 100, 105, 55, 88, 97, 73, 82, 121, 65, 78, 76, 100, 106, 80, 85, 101, 89, 116, 88, 118, 110, 103, 81, 107, 116, 105, 114, 100, 71, 111, 104, 51, 100, 102, 68, 100, 107, 89, 110, 99, 79, 120, 76, 43, 69, 90, 70, 56, 43, 65, 108, 52, 69, 121, 111, 76, 76, 105, 56, 118, 85, 71, 102, 110, 89, 83, 88, 53, 47, 107, 121, 78, 102, 120, 57, 103, 106, 75, 101, 80, 47, 108, 65, 54, 106, 115, 72, 86, 84, 50, 68, 72, 52, 71, 83, 111, 79, 118, 119, 99, 47, 54, 111, 52, 89, 103, 118, 85, 67, 103, 106, 120, 74, 48, 80, 50, 90, 105, 77, 100, 56, 101, 81, 82, 108, 102, 72, 110, 109, 99, 81, 76, 57, 72, 50, 76, 48, 102, 116, 119, 100, 105, 75, 50, 88, 49, 80, 68, 65, 117, 51, 66, 119, 55, 121, 47, 109, 57, 118, 53, 75, 112, 65, 75, 80, 87, 77, 97, 55, 70, 101, 79, 118, 107, 122, 104, 121, 122, 77, 121, 118, 78, 98, 53, 66, 53, 72, 101, 76, 111, 69, 69, 107, 48, 84, 82, 73, 57, 81, 76, 74, 48, 107, 67, 83, 54, 105, 50, 52, 73, 83, 81, 104, 112, 65, 78, 50, 103, 71, 78, 77, 65, 116, 65, 57, 112, 77, 65, 107, 121, 79, 97, 67, 105, 72, 72, 103, 84, 111, 67, 75, 119, 72, 67, 68, 106, 116, 55, 53, 97, 97, 118, 113, 88, 103, 88, 72, 65, 88, 77, 88, 101, 65, 83, 79, 121, 121, 79, 122, 77, 66, 107, 68, 100, 66, 49, 79, 86, 55, 109, 115, 100, 98, 72, 86, 119, 51, 81, 99, 48, 47, 86, 68, 99, 110, 121, 52, 82, 54, 105, 49, 54, 115, 79, 110, 101, 114, 82, 119, 115, 67, 90, 89, 98, 87, 78, 70, 57, 47, 90, 115, 43, 82, 103, 81, 69, 119, 72, 100, 116, 66, 70, 78, 86, 50, 81, 74, 74, 97, 76, 104, 54, 112, 53, 68, 54, 56, 52, 84, 100, 84, 52, 65, 47, 115, 76, 73, 75, 115, 115, 112, 122, 110, 57, 49, 115, 70, 85, 86, 76, 70, 76, 87, 102, 80, 101, 105, 122, 102, 43, 85, 77, 105, 47, 101, 71, 76, 78, 48, 122, 122, 117, 87, 79, 111, 68, 51, 119, 84, 79, 69, 66, 122, 54, 121, 103, 115, 113, 79, 111, 55, 70, 78, 101, 43, 77, 106, 43, 66, 50, 89, 76, 65, 99, 68, 120, 78, 53, 55, 69, 77, 51, 78, 52, 118, 81, 108, 85, 49, 103, 99, 118, 122, 100, 88, 88, 118, 57, 73, 47, 69, 43, 106, 88, 101, 102, 113, 98, 72, 55, 90, 80, 65, 72, 68, 113, 82, 82, 100, 108, 110, 115, 102, 80, 106, 113, 80, 48, 55, 107, 50, 84, 66, 98, 111, 103, 56, 106, 74, 119, 76, 79, 84, 79, 57, 49, 122, 115, 102, 77, 105, 108, 122, 109, 87, 48, 105, 106, 65, 75, 106, 71, 99, 120, 98, 112, 102, 99, 87, 101, 76, 69, 115, 84, 47, 98, 67, 81, 67, 87, 57, 52, 49, 120, 97, 43, 119, 120, 98, 116, 51, 50, 71, 106, 51, 51, 85, 89, 122, 110, 70, 104, 52, 73, 75, 47, 50, 110, 80, 103, 77, 72, 68, 66, 103, 80, 101, 105, 55, 107, 122, 114, 79, 99, 79, 80, 90, 70, 67, 86, 100, 56, 90, 79, 77, 101, 73, 111, 54, 101, 73, 104, 107, 53, 84, 67, 119, 80, 52, 82, 113, 80, 77, 67, 51, 77, 108, 84, 117, 86, 57, 110, 78, 112, 101, 113, 65, 117, 67, 105, 106, 110, 56, 47, 86, 56, 80, 83, 120, 51, 107, 77, 108, 67, 57, 86, 117, 117, 109, 97, 110, 85, 79, 116, 81, 79, 108, 53, 89, 79, 86, 119, 54, 43, 120, 79, 122, 77, 88, 47, 85, 86, 68, 106, 68, 86, 80, 68, 120, 113, 104, 116, 110, 75, 119, 102, 86, 49, 86, 97, 99, 98, 50, 90, 99, 116, 109, 109, 55, 56, 70, 115, 114, 77, 67, 103, 101, 118, 65, 55, 43, 65, 47, 73, 55, 108, 72, 70, 97, 87, 81, 90, 97, 120, 56, 106, 101, 87, 76, 115, 103, 97, 83, 73, 73, 108, 65, 72, 69, 71, 54, 54, 115, 73, 78, 111, 56, 56, 117, 85, 55, 117, 102, 69, 85, 117, 80, 50, 75, 100, 48, 43, 83, 121, 121, 79, 70, 118, 101, 48, 78, 67, 71, 73, 55, 48, 51, 49, 57, 90, 70, 111, 105, 106, 116, 55, 109, 43, 52, 50, 72, 79, 55, 51, 88, 47, 66, 57, 105, 50, 52, 104, 43, 71, 104, 90, 119, 90, 112, 75, 107, 108, 68, 65, 65, 65, 65, 65, 66, 74, 82, 85, 53, 69, 114, 107, 74, 103, 103, 103, 61, 61 };

namespace SE::Editor
{
    EditorUI::~EditorUI()
    {
        ENGINE_ASSERT(m_editorTools.IsEmpty());
        // ENGINE_ASSERT(m_pMapEditor == nullptr);
        // ENGINE_ASSERT(m_pGamePreviewer == nullptr);

        ENGINE_ASSERT(m_pRenderingSystem == nullptr);
        // ENGINE_ASSERT( m_pWorldManager == nullptr );
    }

    void EditorUI::SetStartupMap(ResID const &mapID)
    {
        ENGINE_ASSERT(mapID.IsValid());

        // if ( mapID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() )
        // {
        //     m_startupMapResourceID = mapID;
        // }
        // else
        {
            LOG_ERROR("Editor", "Invalid startup map resource supplied: (0)", m_startupMapResourceID.c_str());
        }
    }

    void EditorUI::Initialize(UpdateContext const &context)
    {
        // ImGui
        //-------------------------------------------------------------------------

        m_editorWindowClass.ClassId = ImHashStr("EditorWindowClass");
        m_editorWindowClass.DockingAllowUnclassed = false;
        m_editorWindowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
        m_editorWindowClass.ViewportFlagsOverrideClear = ImGuiViewportFlags_NoDecoration | ImGuiViewportFlags_NoTaskBarIcon;
        m_editorWindowClass.ParentViewportId = 0; // Top level window
        m_editorWindowClass.DockingAllowUnclassed = false;
        m_editorWindowClass.DockingAlwaysTabBar = true;

        // Systems
        //-------------------------------------------------------------------------

        // m_pWorldManager = context.GetSystem<EntityWorldManager>();
        m_pRenderingSystem = SystemRegistry::GetSystem<Render::RenderingSystem>();
        auto pTaskSystem = SystemRegistry::GetSystem<TaskSystem>();

        // Resources
        //-------------------------------------------------------------------------
        ResourceGlobalSettings resourcesSettings = SystemRegistry::GetSystem<ResourceSystem>()->GetSettings();

        m_resourceDB.Initialize(pTaskSystem, resourcesSettings.m_rawResourcePath, resourcesSettings.m_compiledResourcePath);
        // 绑定资源卸载事件
        m_resourceDeletedEventID = m_resourceDB.AddResourceDeletedCallBack([this] ( ResID  const& resourceID )
        { 
            OnResourceDeleted( resourceID );
        });
        pResourceDatabase = &m_resourceDB;

        // Icons/Images
        //-------------------------------------------------------------------------

        m_editorIcon = ImGui::ImageCache::LoadImageFromMemoryBase64(g_editorIcon, g_editorIconSize);

        // Map Editor
        //-------------------------------------------------------------------------

        auto &request = m_editorToolCreationRequests.AddOne();
        request.type = ToolCreationRequest::MapEditor;

        // Create default editor tools
        //-------------------------------------------------------------------------

        CreateTool<ConsoleWindow>(this);
        CreateTool<FileBrowserWindow>(this);
    }

    void EditorUI::Shutdown(UpdateContext const &context)
    {
        // Map Editor
        //-------------------------------------------------------------------------

        // m_pMapEditor->OnGamePreviewStartRequested().Unbind( m_gamePreviewStartRequestEventBindingID );
        // m_pMapEditor->OnGamePreviewStopRequested().Unbind( m_gamePreviewStopRequestEventBindingID );

        // ENGINE_ASSERT(m_pMapEditor != nullptr);
        // m_pMapEditor = nullptr;
        // m_pGamePreviewer = nullptr;

        // Editor Tools
        //-------------------------------------------------------------------------

        for (auto &creationRequest : m_editorToolCreationRequests)
        {
            if (creationRequest.type == ToolCreationRequest::UninitializedTool)
            {
                Delete(creationRequest.pEditorTool);
            }
        }

        while (!m_editorTools.IsEmpty())
        {
            DestroyTool(context, m_editorTools[0], true);
        }

        // Misc
        //-------------------------------------------------------------------------

        ImGui::ImageCache::UnloadImage(m_editorIcon);

        // Resources
        //-------------------------------------------------------------------------

        pResourceDatabase = nullptr;
        m_resourceDB.RemoveResourceDeletedCallBack(m_resourceDeletedEventID);
        m_resourceDB.Shutdown();

        // Systems
        //-------------------------------------------------------------------------

        // m_pWorldManager = nullptr;
        m_pRenderingSystem = nullptr;
    }

    bool EditorUI::TryOpenResource(ResID  const &resourceID) const
    {
        if (resourceID.IsValid())
        {
            const_cast<EditorUI *>(this)->QueueCreateTool(resourceID);
            return true;
        }

        return false;
    }

    bool EditorUI::TryFindInResourceBrowser(ResID  const &resourceID) const
    {
        // auto pResourceBrowser = GetTool<ResourceBrowserEditorTool>();
        // if ( pResourceBrowser == nullptr )
        // {
        //     pResourceBrowser = const_cast<EditorUI*>(this)->CreateTool<ResourceBrowserEditorTool>( this );
        // }

        //-------------------------------------------------------------------------

        // pResourceBrowser->TryFindAndSelectResource( resourceID );
        return true;
    }

    //-------------------------------------------------------------------------
    // Title bar
    //-------------------------------------------------------------------------

    void EditorUI::GetBorderlessTitleBarInfo(Math::ScreenSpaceRectangle &outTitlebarRect, bool &isInteractibleWidgetHovered) const
    {
        outTitlebarRect = m_titleBar.GetScreenRectangle();
        isInteractibleWidgetHovered = ImGui::IsAnyItemHovered();
    }

    void EditorUI::DrawTitleBarMenu(UpdateContext const &context)
    {
        ImGui::Image(m_editorIcon);

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        if (ImGui::BeginMenu("File"))
        {
            /* code */

            ImGui::EndMenu();
        }
        

        if (ImGui::BeginMenu("Window"))
        {
            ImGui::TextSeparator("Resource");

            bool isFileBrowserOpen = GetTool<FileBrowserWindow>() != nullptr;
            if (ImGui::MenuItem("FileBrowser", nullptr, &isFileBrowserOpen, !isFileBrowserOpen))
            {
                CreateTool<FileBrowserWindow>(this);
            }

            bool isResourceSystemOpen = GetTool<ResourceSystemWindow>() != nullptr;
            if (ImGui::MenuItem("Resource System", nullptr, &isResourceSystemOpen, !isResourceSystemOpen))
            {
                CreateTool<ResourceSystemWindow>(this);
            }

            // bool isResourceImporterOpen = GetTool<ResourceImporterEditorTool>() != nullptr;
            // if (SGUI::MenuItem("Resource Importer", nullptr, &isResourceImporterOpen, !isResourceImporterOpen))
            // {
            //     // CreateTool<ResourceImporterEditorTool>(this);
            // }

            if (SGUI::MenuItem("Rebuild Resource Database"))
            {
                // m_resourceDB.RequestRebuild();
            }

            //-------------------------------------------------------------------------

            SGUI::TextSeparator("System");

            // bool isLogOpen = GetTool<SystemLogEditorTool>() != nullptr;
            // if (SGUI::MenuItem("System Log", nullptr, &isLogOpen, !isLogOpen))
            // {
            //     CreateTool<SystemLogEditorTool>(this);
            // }

            if (SGUI::MenuItem("Open Profiler"))
            {
                Profiling::OpenProfiler();
            }

            //-------------------------------------------------------------------------

            SGUI::TextSeparator("Editor");

            m_isUITestWindowOpen = SGUI::MenuItem("UI Test Window", nullptr, m_isUITestWindowOpen);
            m_isImguiDemoWindowOpen = SGUI::MenuItem("ImGui Demo Window", nullptr, m_isImguiDemoWindowOpen);
            // SGUI::MenuItem("ImGui Plot Demo Window", nullptr, &m_isImguiPlotDemoWindowOpen, !m_isImguiPlotDemoWindowOpen);

            //-------------------------------------------------------------------------

            ImGui::EndMenu();
        }
    }

    void EditorUI::DrawTitleBarInfoStats(UpdateContext const &context)
    {
        auto pResourceSystem = SystemRegistry::GetSystem<ResourceSystem>();
        if (pResourceSystem->IsBusy())
        {
            ImGui::SameLine();
            // if (SGUI::DrawSpinner("##RS", Colors::LimeGreen))
            // {
            //     CreateTool<ResourceSystemEditorTool>(this);
            // }
            // SGUI::TextTooltip("Resource System Busy");
        }
        else
        {
            // if (SGUI::FlatButton(ENGINE_ICON_SLEEP))
            // {
            //     CreateTool<ResourceSystemEditorTool>(this);
            // }
            // SGUI::TextTooltip("Resource System Idle");
        }

        ImGui::SameLine();
        // SystemDebugView::DrawFrameLimiterCombo(const_cast<UpdateContext &>(context));
        // SGUI::ItemTooltip("Frame Limiter");
        

        ImGui::SameLine();
        float const currentFPS = 1.0f / context.GetDeltaTime();
        SGUI::Label(String::Format("FPS: {:.2f}", currentFPS).Get());

        ImGui::SameLine();
        float const allocatedMemory = Memory::GetTotalAllocatedMemory() / 1024.0f / 1024.0f;
        SGUI::Label(String::Format("MEM: {:.2f}MB", allocatedMemory).Get());
    }

    //-------------------------------------------------------------------------
    // Update
    //-------------------------------------------------------------------------

    void EditorUI::StartFrame(UpdateContext const &context)
    {
        UpdateStage const updateStage = context.GetUpdateStage();
        ENGINE_ASSERT(updateStage == UpdateStage::FrameStart);

        //-------------------------------------------------------------------------
        // Resource Systems
        //-------------------------------------------------------------------------

        m_resourceDB.Update();

        //-------------------------------------------------------------------------
        // Handle Warnings/Errors
        //-------------------------------------------------------------------------

        auto const unhandledWarningsAndErrors = Log::System::GetUnhandledWarningsAndErrors();
        if (!unhandledWarningsAndErrors.IsEmpty())
        {
            CreateTool<ConsoleWindow>(this);
        }

        //-------------------------------------------------------------------------
        // Editor Tool Management
        //-------------------------------------------------------------------------

        // Destroy all required editor tools
        // We needed to defer this to the start of the update since we may have references resources that we might unload (i.e. textures)
        for (auto pEditorToolToDestroy : m_editorToolDestructionRequests)
        {
            if (m_pLastActiveTool == pEditorToolToDestroy)
            {
                m_pLastActiveTool = nullptr;
            }

            DestroyTool(context, pEditorToolToDestroy);
        }
        m_editorToolDestructionRequests.Clear();

        // Create all editor tools
        for (ToolCreationRequest const &request : m_editorToolCreationRequests)
        {
            TryCreateTool(context, request);
        }
        m_editorToolCreationRequests.Clear();

        //-------------------------------------------------------------------------
        // Title Bar
        //-------------------------------------------------------------------------
        auto TitleBarLeftContents = [this, &context]()
        {
            this->DrawTitleBarMenu(context);
        };

        auto TitleBarRightContents = [this, &context]()
        {
            this->DrawTitleBarInfoStats(context);
        };

        m_titleBar.Draw(TitleBarLeftContents, 210, TitleBarRightContents, 250);

        //-------------------------------------------------------------------------
        // Create main dock window
        //-------------------------------------------------------------------------

        ImGuiID const dockspaceID = ImGui::GetID("EditorDockSpace");

        ImGuiViewport const *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags const windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking |
                                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("EditorDockSpaceWindow", nullptr, windowFlags);
        ImGui::PopStyleVar(3);
        {
            // Create initial layout
            if (!ImGui::DockBuilderGetNode(dockspaceID))
            {
                ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetContentRegionAvail());

                ImGuiID topDockID = 0, topRightDockID = 0, topLeftDockID = 0, bottomDockID = 0;
                ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Down, 0.1f, &bottomDockID, &topDockID);
                ImGui::DockBuilderSplitNode(topDockID, ImGuiDir_Left, 0.25f, &topLeftDockID, &topRightDockID);
                ImGui::DockBuilderFinish(dockspaceID);

                //-------------------------------------------------------------------------
                auto pConsoleWindow = GetTool<ConsoleWindow>();
                ImGui::DockBuilderDockWindow(pConsoleWindow->GetDisplayName(), bottomDockID);

                auto pResourceBrowser = GetTool<FileBrowserWindow>();
                ImGui::DockBuilderDockWindow(pResourceBrowser->GetDisplayName(), bottomDockID);

                // ImGui::DockBuilderDockWindow(m_pMapEditor->m_windowName.c_str(), rightDockID);
            }

            // Create the actual dock space
            ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);
            ImGui::DockSpace(dockspaceID, viewport->WorkSize, 0, &m_editorWindowClass);
            ImGui::PopStyleVar(1);
        }
        ImGui::End();

        //-------------------------------------------------------------------------
        // Draw editor windows
        //-------------------------------------------------------------------------

        if (m_isImguiDemoWindowOpen)
        {
            ImGui::ShowDemoWindow(&m_isImguiDemoWindowOpen);
        }

        if (m_isImguiPlotDemoWindowOpen)
        {
            // ImPlot::ShowDemoWindow(&m_isImguiPlotDemoWindowOpen);
        }

        if (m_isUITestWindowOpen)
        {
            DrawUITestWindow();
        }

        //-------------------------------------------------------------------------
        // Draw open editor tools
        //-------------------------------------------------------------------------

        EditorWindow *pEditorToolToClose = nullptr;

        // Clear the modal dialog flag - this is done to ensure we only get one modal dialog at a time
        m_hasOpenModalDialog = false;

        // Update the location for all editor tools
        for (auto pEditorTool : m_editorTools)
        {
            if (!SubmitToolMainWindow(context, pEditorTool, dockspaceID))
            {
                pEditorToolToClose = pEditorTool;
            }
        }

        // Draw all editor tools
        for (auto pEditorTool : m_editorTools)
        {
            // If we've been asked to close, no point drawing
            if (pEditorTool == pEditorToolToClose)
            {
                continue;
            }

            // The game previewer is special and is handled separately
            // if (pEditorTool == m_pGamePreviewer)
            // {
            //     continue;
            // }

            // Dont draw any editor tools queued for destructor
            if (m_editorToolDestructionRequests.Contains(pEditorTool))
            {
                continue;
            }

            DrawToolContents(context, pEditorTool);
        }

        // Did we get a close request?
        if (pEditorToolToClose != nullptr)
        {
            // We need to defer this to the start of the update since we may have references resources that we might unload (i.e. textures)
            QueueDestroyTool(pEditorToolToClose);
        }
    }

    void EditorUI::EndFrame(UpdateContext const &context)
    {
        // Game previewer needs to be drawn at the end of the frames since then all the game simulation data will be correct and all the debug tools will be accurate
        // if (m_pGamePreviewer != nullptr && !ListContains(m_editorToolDestructionRequests, m_pGamePreviewer))
        // {
        //     DrawToolContents(context, m_pGamePreviewer);
        //     m_pGamePreviewer->DrawEngineDebugUI(context);
        // }
    }

    void EditorUI::Update(UpdateContext const &context)
    {
        for (auto pEditorTool : m_editorTools)
        {
            // if (pEditorTool->HasEntityWorld())
            // {
            //     EntityWorldUpdateContext updateContext(context, pEditorTool->GetEntityWorld());
            //     pEditorTool->PreWorldUpdate(updateContext);
            // }
        }
    }

    //-------------------------------------------------------------------------
    // Hot Reload
    //-------------------------------------------------------------------------

    void EditorUI::HotReload_UnloadResources(List<ResourceRequesterID> const &usersToBeReloaded, List<ResID > const &resourcesToBeReloaded)
    {
        for (auto pEditorTool : m_editorTools)
        {
            // pEditorTool->HotReload_UnloadResources(usersToBeReloaded, resourcesToBeReloaded);
        }

        // if (m_pGamePreviewer != nullptr)
        // {
        //     m_pGamePreviewer->m_pDebugUI->HotReload_UnloadResources(usersToBeReloaded, resourcesToBeReloaded);
        // }
    }

    void EditorUI::HotReload_ReloadResources()
    {
        for (auto pEditorTool : m_editorTools)
        {
            // pEditorTool->HotReload_ReloadResources();

            // Auto destroy any editor tools that had a problem loading their descriptor i.e. they were externally corrupted.
            // if (pEditorTool->IsResourceEditor() && !pEditorTool->IsDescriptorLoaded())
            // {
            //     String const str(String::CtorSprintf(), "There was an error reloading the descriptor for editor tool: %s! Please check the log for details.", pEditorTool->GetDisplayName());
            //     pfd::message("Error Loading Descriptor", str.c_str(), pfd::choice::ok, pfd::icon::error).result();
            //     QueueDestroyTool(pEditorTool);
            // }
        }

        // if (m_pGamePreviewer != nullptr)
        // {
        //     m_pGamePreviewer->m_pDebugUI->HotReload_ReloadResources();
        // }
    }

    //-------------------------------------------------------------------------
    // Resource Management
    //-------------------------------------------------------------------------

    void EditorUI::OnResourceDeleted(ResID  const &resourceID)
    {
        ENGINE_ASSERT(resourceID.IsValid());

        for (auto pEditorTool : m_editorTools)
        {
            // if (pEditorTool->HasDependencyOnResource(resourceID))
            // {
            //     QueueDestroyTool(pEditorTool);
            // }
        }
    }

    //-------------------------------------------------------------------------
    // EditorWindow Management
    //-------------------------------------------------------------------------

    bool EditorUI::TryCreateTool(UpdateContext const &context, ToolCreationRequest const &request)
    {
        // Uninitialized EditorWindow
        //-------------------------------------------------------------------------

        if (request.type == ToolCreationRequest::UninitializedTool)
        {
            request.pEditorTool->Initialize(context);
            m_editorTools.Add(request.pEditorTool);
            return true;
        }

        // Map editor
        //-------------------------------------------------------------------------

        else if (request.type == ToolCreationRequest::MapEditor)
        {
            // Destroy the default created game world
            // m_pWorldManager->DestroyWorld(m_pWorldManager->GetWorlds()[0]);

            // // Create a new editor world for the map editor editor tool
            // auto pMapEditorWorld = m_pWorldManager->CreateWorld(EntityWorldType::Tools);
            // m_pRenderingSystem->CreateCustomRenderTargetForViewport(pMapEditorWorld->GetViewport(), true);

            // // Create the map editor editor tool
            // m_pMapEditor = EE::New<EntityModel::EntityMapEditor>(this, pMapEditorWorld);
            // m_pMapEditor->Initialize(context);
            // m_editorTools.emplace_back(m_pMapEditor);

            // m_gamePreviewStartRequestEventBindingID = m_pMapEditor->OnGamePreviewStartRequested().Bind([this](UpdateContext const &context)
            //                                                                                            { CreateGamePreviewTool(context); });
            // m_gamePreviewStopRequestEventBindingID = m_pMapEditor->OnGamePreviewStopRequested().Bind([this](UpdateContext const &context)
            //                                                                                          { DestroyGamePreviewTool(context); });

            // // Load startup map
            // if (m_startupMapResourceID.IsValid())
            // {
            //     ENGINE_ASSERT(m_startupMapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID());
            //     m_pMapEditor->LoadMap(m_startupMapResourceID);
            // }

            return true;
        }

        // Game previewer
        //-------------------------------------------------------------------------

        else if (request.type == ToolCreationRequest::GamePreview)
        {
            // auto pPreviewWorld = m_pWorldManager->CreateWorld(EntityWorldType::Game);
            // m_pRenderingSystem->CreateCustomRenderTargetForViewport(pPreviewWorld->GetViewport());
            // m_pGamePreviewer = EE::New<GamePreviewer>(this, pPreviewWorld);
            // m_pGamePreviewer->Initialize(context);
            // m_pGamePreviewer->LoadMapToPreview(m_pMapEditor->GetLoadedMap());
            // m_editorTools.emplace_back(m_pGamePreviewer);

            // m_pMapEditor->NotifyGamePreviewStarted();

            return true;
        }

        // Resource
        //-------------------------------------------------------------------------

        else if (request.type == ToolCreationRequest::ResourceEditor)
        {
            ENGINE_ASSERT(request.resourceID.IsValid());
            ResTypeID const resourceTypeID = request.resourceID.GetResourceTypeID();

            // Don't try to open invalid resource IDs
            // if (!m_resourceDB.DoesResourceExist(request.m_resourceID))
            // {
            //     return false;
            // }

            // Handle maps explicitly
            //-------------------------------------------------------------------------

            // if (resourceTypeID == EntityModel::SerializedEntityMap::GetStaticResourceTypeID())
            // {
            //     m_pMapEditor->LoadMap(request.m_resourceID);
            //     SGUI::MakeTabVisible(m_pMapEditor->m_windowName.c_str());
            //     return true;
            // }

            // Other resource types
            //-------------------------------------------------------------------------

            // Check if we already have a editor tool open for this resource, if so then switch focus to it
            for (auto pEditorTool : m_editorTools)
            {
                if (pEditorTool->IsEditingResource(request.resourceID))
                {
                    SGUI::MakeTabVisible(pEditorTool->m_windowName.Get());
                    return true;
                }
            }

            // Check if we can create a new editor tool
            if (!ResourceEditorFactory::CanCreateEditor(this, request.resourceID))
            {
                return false;
            }

            // Create tools world
            // auto pEditorToolWorld = m_pWorldManager->CreateWorld(EntityWorldType::Tools);
            // m_pRenderingSystem->CreateCustomRenderTargetForViewport(pEditorToolWorld->GetViewport(), true);

            // Create editor tool
            // auto pCreatedTool = ResourceEditorFactory::CreateEditor(this, pEditorToolWorld, request.m_resourceID);
            // m_editorTools.emplace_back(pCreatedTool);

            // Initialize editor tool
            // pCreatedTool->Initialize(context);

            // Check if the descriptor was correctly loaded, if not schedule this editor tool to be destroyed
            // if (!pCreatedTool->IsDescriptorLoaded())
            // {
            //     String const str(String::CtorSprintf(), "There was an error loading the descriptor for %s! Please check the log for details.", request.m_resourceID.c_str());
            //     pfd::message("Error Loading Descriptor", str.c_str(), pfd::choice::ok, pfd::icon::error).result();
            //     DestroyTool(context, pCreatedTool);
            //     return false;
            // }

            return true;
        }
        else
        {
            ENGINE_UNREACHABLE_CODE();
        }

        //-------------------------------------------------------------------------

        return false;
    }

    void EditorUI::QueueCreateTool(ResID  const &resourceID)
    {
        auto &request = m_editorToolCreationRequests.AddOne();
        request.type = ToolCreationRequest::ResourceEditor;
        request.resourceID = resourceID;
    }

    void EditorUI::DestroyTool(UpdateContext const &context, EditorWindow *pEditorTool, bool isEditorShutdown)
    {
        // ENGINE_ASSERT(m_pMapEditor != pEditorTool);
        ENGINE_ASSERT(pEditorTool != nullptr);

        auto foundToolIndex = m_editorTools.Find(pEditorTool);
        ENGINE_ASSERT(foundToolIndex != INVALID_INDEX);

        // bool const isGamePreviewerTool = m_pGamePreviewer == pEditorTool;

        // Save unsaved changes
        //-------------------------------------------------------------------------

        if (pEditorTool->SupportsSaving() && pEditorTool->IsDirty())
        {
            // auto messageDialog = pfd::message("Unsaved Changes", "You have unsaved changes!\nDo you wish to save these changes before closing?", isEditorShutdown ? pfd::choice::yes_no : pfd::choice::yes_no_cancel);
            // switch (messageDialog.result())
            // {
            // case pfd::button::yes:
            // {
            //     if (!pEditorTool->Save())
            //     {
            //         return;
            //     }
            // }
            // break;

            // case pfd::button::cancel:
            // {
            //     return;
            // }
            // break;

            // case pfd::button::no:
            // default:
            // {
            //     // Do Nothing
            // }
            // break;
            // }
        }

        //-------------------------------------------------------------------------

        // Get the world before we destroy the editor tool
        auto pEditorToolWorld = pEditorTool->GetEntityWorld();

        // Destroy editor tool
        if (pEditorTool->IsInitialized())
        {
            pEditorTool->Shutdown(context);
        }
        Delete(pEditorTool);
        m_editorTools.RemoveAt(foundToolIndex);

        // Clear the game previewer editor tool ptr if we just destroyed it
        // if (isGamePreviewerTool)
        // {
        //     m_pMapEditor->NotifyGamePreviewEnded();
        //     m_pGamePreviewer = nullptr;
        // }

        // Destroy preview world and render target
        if (pEditorToolWorld)
        {
            // m_pRenderingSystem->DestroyCustomRenderTargetForViewport(pEditorToolWorld->GetViewport());
            // m_pWorldManager->DestroyWorld(pEditorToolWorld);
        }
    }

    void EditorUI::QueueDestroyTool(EditorWindow *pEditorTool)
    {
        ENGINE_ASSERT(pEditorTool != nullptr);
        // ENGINE_ASSERT(m_pMapEditor != pEditorTool);
        m_editorToolDestructionRequests.Add(pEditorTool);
    }

    void EditorUI::ToolLayoutCopy(EditorWindow *pEditorTool)
    {
        ImGuiID sourceToolID = pEditorTool->m_previousDockspaceID;
        ImGuiID destinationToolID = pEditorTool->m_currentDockspaceID;
        IM_ASSERT(sourceToolID != 0);
        IM_ASSERT(destinationToolID != 0);

        // Helper to build an array of strings pointer into the same contiguous memory buffer.
        struct ContiguousStringArrayBuilder
        {
            void AddEntry(const char *data, uint64 dataLength)
            {
                int32 const bufferSize = (int32)m_buffer.Count();
                m_offsets.Add(bufferSize);
                int32 const offset = bufferSize;
                m_buffer.Resize(bufferSize + (int32)dataLength);
                memcpy(m_buffer.Get() + offset, data, dataLength);
            }

            void BuildPointerArray(ImVector<const char *> &outArray)
            {
                outArray.resize((int32)m_offsets.Count());
                for (int32 n = 0; n < (int32)m_offsets.Count(); n++)
                {
                    outArray[n] = m_buffer.Get() + m_offsets[n];
                }
            }

            List<char> m_buffer;
            List<int32> m_offsets;
        };

        // Build an array of remapped names
        ContiguousStringArrayBuilder namePairsBuilder;

        // Iterate tool windows
        for (auto &toolWindow : pEditorTool->m_toolWindows)
        {
            String const sourceToolWindowName = EditorWindow::GetToolWindowName(toolWindow.m_name.Get(), sourceToolID);
            String const destinationToolWindowName = EditorWindow::GetToolWindowName(toolWindow.m_name.Get(), destinationToolID);
            namePairsBuilder.AddEntry(sourceToolWindowName.Get(), sourceToolWindowName.Length() + 1);
            namePairsBuilder.AddEntry(destinationToolWindowName.Get(), destinationToolWindowName.Length() + 1);
        }

        // Build the same array with char* pointers at it is the input of DockBuilderCopyDockspace() (may change its signature?)
		ImVector<const char *> windowRemapPairs;
        namePairsBuilder.BuildPointerArray(windowRemapPairs);

        // Perform the cloning
        ImGui::DockBuilderCopyDockSpace(sourceToolID, destinationToolID, &windowRemapPairs);
        ImGui::DockBuilderFinish(destinationToolID);
    }

    bool EditorUI::SubmitToolMainWindow(UpdateContext const &context, EditorWindow *pEditorTool, ImGuiID editorDockspaceID)
    {
        ENGINE_ASSERT(pEditorTool != nullptr && pEditorTool->IsInitialized());
        IM_ASSERT(editorDockspaceID != 0);

        bool isToolStillOpen = true;
        bool *pIsToolOpen = &isToolStillOpen; // (pEditorTool == m_pMapEditor) ? nullptr : &isToolStillOpen; // Prevent closing the map-editor editor tool

        // Top level editors can only be docked with each others
        ImGui::SetNextWindowClass(&m_editorWindowClass);
        if (pEditorTool->m_desiredDockID != 0)
        {
            ImGui::SetNextWindowDockID(pEditorTool->m_desiredDockID);
            pEditorTool->m_desiredDockID = 0;
        }
        else
        {
            ImGui::SetNextWindowDockID(editorDockspaceID, ImGuiCond_FirstUseEver);
        }

        // Window flags
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;

        if (pEditorTool->SupportsMainMenu())
        {
            windowFlags |= ImGuiWindowFlags_MenuBar;
        }

        if (pEditorTool->IsDirty())
        {
            windowFlags |= ImGuiWindowFlags_UnsavedDocument;
        }

        //-------------------------------------------------------------------------

        ImGuiWindow *pCurrentWindow = ImGui::FindWindowByName(pEditorTool->m_windowName.Get());
        bool const isVisible = pCurrentWindow != nullptr && !pCurrentWindow->Hidden;

        // Create top level editor tab/window
        ImGui::PushStyleColor(ImGuiCol_Text, isVisible ? SGUI::Style::s_colorAccent0 : SGUI::Style::s_colorText);
        ImGui::SetNextWindowSizeConstraints(ImVec2(128, 128), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::Begin(pEditorTool->m_windowName.Get());
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        // Store last focused document
        bool const isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy);
        if (isFocused)
        {
            m_pLastActiveTool = pEditorTool;
        }

        // Set WindowClass based on per-document ID, so tabs from Document A are not dockable in Document B etc. We could be using any ID suiting us, e.g. &pEditorTool
        // We also set ParentViewportId to request the platform back-end to set parent/child relationship at the windowing level.
        pEditorTool->m_toolWindowClass.ClassId = pEditorTool->m_ID;
        pEditorTool->m_toolWindowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoTaskBarIcon | ImGuiViewportFlags_NoDecoration;
        pEditorTool->m_toolWindowClass.ParentViewportId = ImGui::GetWindowViewport()->ID; // Make child of the top-level editor window
        pEditorTool->m_toolWindowClass.DockingAllowUnclassed = true;

        // Track LocationID change so we can fork/copy the layout data according to where the window is going + reference count
        // LocationID ~~ (DockId != 0 ? DockId : DocumentID) // When we are in a loose floating window we use our own document id instead of the dock id
        pEditorTool->m_currentDockID = ImGui::GetWindowDockID();
        pEditorTool->m_previousLocationID = pEditorTool->m_currentLocationID;
        pEditorTool->m_currentLocationID = pEditorTool->m_currentDockID != 0 ? pEditorTool->m_currentDockID : pEditorTool->m_ID;

        // Dockspace ID ~~ Hash of LocationID + DocType
        // So all editors of a same type inside a same tab-bar will share the same layout.
        // We will also use this value as a suffix to create window titles, but we could perfectly have an indirection to allocate and use nicer names for window names (e.g. 0001, 0002).
        pEditorTool->m_previousDockspaceID = pEditorTool->m_currentDockspaceID;
        pEditorTool->m_currentDockspaceID = pEditorTool->CalculateDockspaceID();
        ENGINE_ASSERT(pEditorTool->m_currentDockspaceID != 0);

        ImGui::End();

        //-------------------------------------------------------------------------

        return isToolStillOpen;
    }

    void EditorUI::DrawToolContents(UpdateContext const &context, EditorWindow *pEditorTool)
    {
        ENGINE_ASSERT(pEditorTool != nullptr && pEditorTool->IsInitialized());

        auto pWorld = pEditorTool->GetEntityWorld();

        //-------------------------------------------------------------------------

        // This is the second Begin(), as SubmitToolWindow() has already done one
        // (Therefore only the p_open and flags of the first call to Begin() applies)
        ImGui::Begin(pEditorTool->m_windowName.Get());
        int32 const beginCount = ImGui::GetCurrentWindow()->BeginCount;
        IM_ASSERT(beginCount == 2);

        ImGuiID const dockspaceID = pEditorTool->m_currentDockspaceID;
        ImVec2 const dockspaceSize = ImGui::GetContentRegionAvail();

        // Fork settings when extracting to a new location, or Overwrite settings when docking back into an existing location
        if (pEditorTool->m_previousLocationID != 0 && pEditorTool->m_previousLocationID != pEditorTool->m_currentLocationID)
        {
            // Count references to tell if we should Copy or Move the layout.
            int32 previousDockspaceRefCount = 0;
            int32 currentDockspaceRefCount = 0;
            for (int32 i = 0; i < m_editorTools.Count(); i++)
            {
                EditorWindow *pOtherTool = m_editorTools[i];

                if (pOtherTool->m_currentDockspaceID == pEditorTool->m_previousDockspaceID)
                {
                    previousDockspaceRefCount++;
                }

                if (pOtherTool->m_currentDockspaceID == pEditorTool->m_currentDockspaceID)
                {
                    currentDockspaceRefCount++;
                }
            }

            // Fork or overwrite settings
            // FIXME: should be able to do a "move window but keep layout" if curr_dockspace_ref_count > 1.
            // FIXME: when moving, delete settings of old windows
            ToolLayoutCopy(pEditorTool);

            if (previousDockspaceRefCount == 0)
            {
                ImGui::DockBuilderRemoveNode(pEditorTool->m_previousDockspaceID);

                // Delete settings of old windows
                // Rely on window name to ditch their .ini settings forever..
                char windowSuffix[16];
                ImFormatString(windowSuffix, IM_ARRAYSIZE(windowSuffix), "##%08X", pEditorTool->m_previousDockspaceID);
                uint64 windowSuffixLength = strlen(windowSuffix);
                ImGuiContext &g = *GImGui;
                for (ImGuiWindowSettings *settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
                {
                    if (settings->ID == 0)
                    {
                        continue;
                    }

                    char const *pWindowName = settings->GetName();
                    uint64 windowNameLength = strlen(pWindowName);
                    if (windowNameLength >= windowSuffixLength)
                    {
                        if (strcmp(pWindowName + windowNameLength - windowSuffixLength, windowSuffix) == 0) // Compare suffix
                        {
                            ImGui::ClearWindowSettings(pWindowName);
                        }
                    }
                }
            }
        }
        else if (ImGui::DockBuilderGetNode(pEditorTool->m_currentDockspaceID) == nullptr)
        {
            ImGui::DockBuilderAddNode(pEditorTool->m_currentDockspaceID, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(pEditorTool->m_currentDockspaceID, dockspaceSize);
            if (!pEditorTool->IsSingleWindowTool())
            {
                pEditorTool->InitializeDockingLayout(dockspaceID, dockspaceSize);
            }
            ImGui::DockBuilderFinish(dockspaceID);
        }

        // FIXME-DOCK: This is a little tricky to explain but we currently need this to use the pattern of sharing a same dockspace between tabs of a same tab bar
        bool isVisible = true;
        if (ImGui::GetCurrentWindow()->Hidden)
        {
            isVisible = false;
        }

        // Update editor tool
        //-------------------------------------------------------------------------

        bool const isLastFocusedTool = (m_pLastActiveTool == pEditorTool);
        pEditorTool->SharedUpdate(context, isVisible, isLastFocusedTool);
        pEditorTool->Update(context, isVisible, isLastFocusedTool);
        pEditorTool->m_isViewportFocused = false;
        pEditorTool->m_isViewportHovered = false;

        // Check Visibility
        //-------------------------------------------------------------------------

        if (!isVisible)
        {
            if (!pEditorTool->IsSingleWindowTool())
            {
                // Keep alive document dockspace so windows that are docked into it but which visibility are not linked to the dockspace visibility won't get undocked.
                ImGui::DockSpace(dockspaceID, dockspaceSize, ImGuiDockNodeFlags_KeepAliveOnly, &pEditorTool->m_toolWindowClass);
            }

            ImGui::End();

            // Suspend world updates for hidden windows
            // if (pEditorTool->HasEntityWorld() && pEditorTool != m_pGamePreviewer)
            // {
            //     pWorld->SuspendUpdates();
            // }

            return;
        }

        // Draw EditorWindow Menu
        //-------------------------------------------------------------------------

        if (pEditorTool->SupportsMainMenu())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 16));
            if (ImGui::BeginMenuBar())
            {
                pEditorTool->DrawMainMenu(context);
                ImGui::EndMenuBar();
            }
            ImGui::PopStyleVar(1);
        }

        // Submit the dockspace node and end window
        //-------------------------------------------------------------------------

        if (pEditorTool->IsSingleWindowTool())
        {
            ENGINE_ASSERT(pEditorTool->m_toolWindows.Count() == 1);
            pEditorTool->m_toolWindows[0].m_drawFunction(context, isLastFocusedTool);
        }
        else
        {
            ImGui::DockSpace(dockspaceID, dockspaceSize, ImGuiDockNodeFlags_None, &pEditorTool->m_toolWindowClass);
        }
        ImGui::End();

        // Manage World state
        //-------------------------------------------------------------------------

        // if (pEditorTool->HasEntityWorld() && (pEditorTool != m_pMapEditor || m_pGamePreviewer == nullptr))
        // {
        //     pWorld->ResumeUpdates();
        // }

        // Draw editor tool tool windows
        //-------------------------------------------------------------------------

        if (!pEditorTool->IsSingleWindowTool())
        {
            for (auto &toolWindow : pEditorTool->m_toolWindows)
            {
                if (!toolWindow.m_isOpen)
                {
                    continue;
                }

                String const toolWindowName = EditorWindow::GetToolWindowName(toolWindow.m_name.Get(), pEditorTool->m_currentDockspaceID);

                // When multiple documents are open, floating tools only appear for focused one
                if (!isLastFocusedTool)
                {
                    if (ImGuiWindow *pWindow = ImGui::FindWindowByName(toolWindowName.Get()))
                    {
                        if (pWindow->DockNode == nullptr || ImGui::DockNodeGetRootNode(pWindow->DockNode)->ID != dockspaceID)
                        {
                            continue;
                        }
                    }
                }

                //-------------------------------------------------------------------------

                if (toolWindow.m_isViewport)
                {
                    ENGINE_ASSERT(pEditorTool->SupportsViewport());
                    ENGINE_ASSERT(pEditorTool->HasEntityWorld() && pWorld != nullptr);

                    // EditorWindow::ViewportInfo viewportInfo;
                    // viewportInfo.m_pViewportRenderTargetTexture = (void *)&m_pRenderingSystem->GetRenderTargetTextureForViewport(pWorld->GetViewport());
                    // viewportInfo.m_retrievePickingID = CallBack<Render::PickingID, Int2 const &>::Create([this, pWorld](Int2 const &pixelCoords)
                    // { 
                    //     return m_pRenderingSystem->GetViewportPickingID(pWorld->GetViewport(), pixelCoords); 
                    // });

                    ImGuiWindowFlags const viewportWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
                    ImGui::SetNextWindowClass(&pEditorTool->m_toolWindowClass);

                    ImGui::SetNextWindowSizeConstraints(Float2(128, 128), Float2(FLT_MAX, FLT_MAX));
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Float2(0, 0));
                    bool const drawViewportWindow = ImGui::Begin(toolWindowName.Get(), nullptr, viewportWindowFlags);
                    ImGui::PopStyleVar();

                    // if (drawViewportWindow)
                    // {
                    //     pEditorTool->m_isViewportFocused = ImGui::IsWindowFocused();
                    //     pEditorTool->m_isViewportHovered = ImGui::IsWindowHovered();
                    //     pEditorTool->DrawViewport(context, viewportInfo);
                    // }

                    ImGui::End();
                }
                else // Draw the tool window
                {
                    ImGuiWindowFlags toolWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
                    if (toolWindow.m_disableScrolling)
                    {
                        toolWindowFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
                    }

                    ImGui::SetNextWindowClass(&pEditorTool->m_toolWindowClass);

					Float2 padding = toolWindow.HasUserSpecifiedWindowPadding() ?
									 toolWindow.m_windowPadding : Float2(ImGui::GetStyle().WindowPadding);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
                    bool const drawToolWindow = ImGui::Begin(toolWindowName.Get(), &toolWindow.m_isOpen, toolWindowFlags);
                    ImGui::PopStyleVar();

                    if (drawToolWindow)
                    {
                        bool const isToolWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy);
                        toolWindow.m_drawFunction(context, isToolWindowFocused);
                    }

                    ImGui::End();
                }
            }
        }

        // Camera
        //-------------------------------------------------------------------------

        if (pEditorTool->HasEntityWorld())
        {
            pEditorTool->SetCameraUpdateEnabled(pEditorTool->m_isViewportFocused && pEditorTool->m_isViewportHovered);
        }

        // Draw any open dialogs
        //-------------------------------------------------------------------------

        // If we dont already have an open modal dialog, draw any dialogs required
        if (!m_hasOpenModalDialog)
        {
            // m_hasOpenModalDialog = pEditorTool->m_dialogManager.DrawDialog(context);
        }
    }

    void EditorUI::CreateGamePreviewTool(UpdateContext const &context)
    {
        // ENGINE_ASSERT(m_pGamePreviewer == nullptr);
        auto &request = m_editorToolCreationRequests.AddOne();
        request.type = ToolCreationRequest::GamePreview;
    }

    void EditorUI::DestroyGamePreviewTool(UpdateContext const &context)
    {
        // ENGINE_ASSERT(m_pGamePreviewer != nullptr);
        // QueueDestroyTool(m_pGamePreviewer);
    }

    //-------------------------------------------------------------------------
    // Misc
    //-------------------------------------------------------------------------

    void EditorUI::DrawUITestWindow()
    {
        if (ImGui::Begin("UI Test", &m_isUITestWindowOpen))
        {
            {
                GUI::ScopedFont sf(GUI::Font::Tiny);
                ImGui::Text(ICON_FILE_CHECK "This is a test - Tiny");
            }
            {
                GUI::ScopedFont sf(GUI::Font::TinyBold);
                ImGui::Text(ICON_ALERT "This is a test - Tiny Bold");
            }
            {
                GUI::ScopedFont sf(GUI::Font::Small);
                ImGui::Text(ICON_FILE_CHECK "This is a test - Small");
            }
            {
                GUI::ScopedFont sf(GUI::Font::SmallBold);
                ImGui::Text(ICON_ALERT "This is a test - Small Bold");
            }
            {
                GUI::ScopedFont sf(GUI::Font::Medium);
                ImGui::Text(ICON_FILE_CHECK "This is a test - Medium");
            }
            {
                GUI::ScopedFont sf(GUI::Font::MediumBold);
                ImGui::Text(ICON_ALERT "This is a test - Medium Bold");
            }
            {
                GUI::ScopedFont sf(GUI::Font::Large);
                ImGui::Text(ICON_FILE_CHECK "This is a test - Large");
            }
            {
                GUI::ScopedFont sf(GUI::Font::LargeBold);
                ImGui::Text(ICON_CCTV_OFF "This is a test - Large Bold");
            }

            //-------------------------------------------------------------------------

            ImGui::NewLine();

            //-------------------------------------------------------------------------

            {
                GUI::ScopedFont sf(GUI::Font::Small);
                SGUI::ColoredButton(Colors::Green, Colors::White, ICON_PLUS "ADD");
            }
            {
                GUI::ScopedFont sf(GUI::Font::SmallBold);
                SGUI::ColoredButton(Colors::Green, Colors::White, ICON_PLUS "ADD");
            }
            {
                GUI::ScopedFont sf(GUI::Font::Medium);
                SGUI::ColoredButton(Colors::Green, Colors::White, ICON_PLUS "ADD");
            }
            {
                GUI::ScopedFont sf(GUI::Font::MediumBold);
                SGUI::ColoredButton(Colors::Green, Colors::White, ICON_PLUS "ADD");
            }
            {
                GUI::ScopedFont sf(GUI::Font::Large);
                SGUI::ColoredButton(Colors::Green, Colors::White, ICON_PLUS "ADD");
            }
            {
                GUI::ScopedFont sf(GUI::Font::LargeBold);
                SGUI::ColoredButton(Colors::Green, Colors::White, ICON_PLUS "ADD");
            }

            //-------------------------------------------------------------------------

            ImGui::NewLine();

            //-------------------------------------------------------------------------

            {
                GUI::ScopedFont sf(GUI::Font::Small);
                ImGui::Button(ICON_HAIR_DRYER);
                ImGui::SameLine();
                ImGui::Button(ICON_Z_WAVE);
                ImGui::SameLine();
                ImGui::Button(ICON_KANGAROO);
                ImGui::SameLine();
                ImGui::Button(ICON_YIN_YANG);
            }

            {
                GUI::ScopedFont sf(GUI::Font::Medium);
                ImGui::Button(ICON_HAIR_DRYER);
                ImGui::SameLine();
                ImGui::Button(ICON_Z_WAVE);
                ImGui::SameLine();
                ImGui::Button(ICON_KANGAROO);
                ImGui::SameLine();
                ImGui::Button(ICON_YIN_YANG);
            }

            {
                GUI::ScopedFont sf(GUI::Font::Large);
                ImGui::Button(ICON_HAIR_DRYER);
                ImGui::SameLine();
                ImGui::Button(ICON_Z_WAVE);
                ImGui::SameLine();
                ImGui::Button(ICON_KANGAROO);
                ImGui::SameLine();
                ImGui::Button(ICON_YIN_YANG);
            }

            //-------------------------------------------------------------------------

            SGUI::IconButton(ICON_KANGAROO, "Test", Colors::PaleGreen, ImVec2(100, 0));

            SGUI::IconButton(ICON_HOME, "Home", Colors::RoyalBlue, ImVec2(100, 0));

            SGUI::IconButton(ICON_MOVIE_PLAY, "Play", Colors::LightPink, ImVec2(100, 0));

            SGUI::ColoredIconButton(Colors::Green, Colors::White, Colors::Yellow, ICON_KANGAROO, "Test", ImVec2(100, 0));

            SGUI::FlatIconButton(ICON_HOME, "Home", Colors::RoyalBlue, ImVec2(100, 0));

            //-------------------------------------------------------------------------

            ImGui::AlignTextToFramePadding();
            SGUI::SameLineSeparator(20);

            ImGui::SameLine(0, 0);
            ImGui::Text("Test");

            ImGui::SameLine(0, 0);
            SGUI::SameLineSeparator();

            ImGui::SameLine(0, 0);
            ImGui::Text("Test");

            ImGui::SameLine(0, 0);
            SGUI::SameLineSeparator(40);

            ImGui::SameLine(0, 0);
            ImGui::Text("Test");

            ImGui::SameLine(0, 0);
            SGUI::SameLineSeparator();

            ImGui::NewLine();
            ImGui::Separator();

            SGUI::DrawSpinner("S1");
            ImGui::SameLine();
            ImGui::Text("Spinner 1");

            SGUI::DrawSpinner("S2", Colors::Yellow, ImVec2(100, 100), 10);
            ImGui::SameLine();
            ImGui::Text("Spinner 2");

            SGUI::DrawSpinner("S3", Colors::Blue, ImVec2(-1, -1), 5);
            ImGui::SameLine();
            ImGui::Text("Spinner 3");
        }
        ImGui::End();
    }
}