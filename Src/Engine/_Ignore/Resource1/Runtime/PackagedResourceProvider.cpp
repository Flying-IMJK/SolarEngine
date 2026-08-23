#include "PackagedResourceProvider.h"
#include "Runtime/Resource/ResourceRequest.h"
#include "Runtime/Settings/GlobalSettings_Resource.h"

//-------------------------------------------------------------------------

namespace SGE
{
    bool PackagedResourceProvider::IsReady() const
    {
        return true;
    }

    bool PackagedResourceProvider::Initialize()
    {
        return true;
    }

    void PackagedResourceProvider::RequestRawResource(ResourceRequest *pRequest)
    {
        String const resourceFilePath = pRequest->GetResourceID().GetResourcePath().ToFileSystemPath(m_settings.m_compiledResourcePath);
        pRequest->OnRawResourceRequestComplete(resourceFilePath, String());
    }

    void PackagedResourceProvider::CancelRequest(ResourceRequest *pRequest)
    {
        // Do Nothing
    }
}