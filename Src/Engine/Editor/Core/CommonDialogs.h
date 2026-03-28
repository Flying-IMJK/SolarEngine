#pragma once
#include "Editor/API.h"
#include "Core/Types/Strings/String.h"
#include "Core/TypeSystem/TypeID.h"
//-------------------------------------------------------------------------

namespace SE::Editor
{
    // Create a save dialog restricted to a specific extension, return true if the user has selected a valid path, returns false if they cancel or somehow select an invalid path
    SE_API_EDITOR bool SaveDialog(String const& extension, String& outPath, String const& startingPath = String::Empty, String const& friendlyFilterName = String::Empty);

    // Create a save dialog restricted to a specific resource type, return true if the user has selected a valid path, returns false if they cancel or somehow select an invalid path
    SE_API_EDITOR bool SaveDialog(TypeID resourceTypeID, String& outPath, String const& startingPath = String::Empty, String const& friendlyFilterName = String::Empty);
}