#include "CommonDialogs.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Platform/FileSystem.h"


//-------------------------------------------------------------------------

namespace SE::Editor
{
    bool SaveDialog(String const& extension, String& outPath, String const& startingPath, String const& friendlyFilterName )
    {
        ENGINE_ASSERT( !extension.IsEmpty() && extension[0] != '.' );

        outPath.Clear();

        // Select file
        //-------------------------------------------------------------------------

/*        String filterString = String::Format(SE_TEXT("*.{}"), extension);

        auto const selectedFilePath = pfd::save_file( "Save File", startingPath.Get(), { friendlyFilterName.Get(), filterString.Get() } ).result();
        if ( selectedFilePath.empty() )
        {
            return false;
        }

        outPath = selectedFilePath.c_str();

        // Validate selected filename
        //-------------------------------------------------------------------------

        auto pSelectedFileExtension = FileSystem::GetExtension(outPath);
        if ( pSelectedFileExtension == nullptr || extension != pSelectedFileExtension)
        {
            String errorString = String::Format(SE_TEXT("Invalid extension provided! You need to have the .{} extension!"), extension);
            pfd::message( "Error", errorString.Get(), pfd::choice::ok, pfd::icon::error ).result();
            outPath.Clear();
            return false;
        }*/

        //-------------------------------------------------------------------------

        return true;
    }

    bool SaveDialog(TypeID resourceTypeID, String& outPath, String const& startingPath, String const& friendlyFilterName )
    {
        outPath.Clear();

        String const resourceTypeIDString = resourceTypeID.ToString();

        // Select file
        //-------------------------------------------------------------------------

/*        String filterString = String::Format(SE_TEXT("*.{}"), resourceTypeIDString.Get());

        auto const selectedFilePath = pfd::save_file( "Save File", startingPath.Get(), { friendlyFilterName.Get(), filterString.Get() } ).result();
        if ( selectedFilePath.empty() )
        {
            return false;
        }

        outPath = selectedFilePath.c_str();

        // Validate selected filename
        //-------------------------------------------------------------------------

        auto extStr = FileSystem::GetExtension(outPath).ToLower();
        if (extStr.IsEmpty())
        {
            FileSystem::ReplaceExtension(outPath, resourceTypeIDString);
        }
        else
        {
            ResTypeID const newTypeID( extStr.Get() );
            if ( resourceTypeID != newTypeID )
            {
                String errorString = String::Format(SE_TEXT("Invalid extension provided! You need to have the .{} extension!"), resourceTypeIDString);
                pfd::message( "Error", errorString.Get(), pfd::choice::ok, pfd::icon::error ).result();
                outPath.Clear();
                return false;
            }
        }*/

        return true;
    }
}