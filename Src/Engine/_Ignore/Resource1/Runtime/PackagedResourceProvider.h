#pragma once

#include "Runtime/Resource/ResourceProvider.h"

//-------------------------------------------------------------------------

namespace SGE
{
    class ResourceGlobalSettings;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME PackagedResourceProvider final : public ResourceProvider
    {

    public:

        PackagedResourceProvider( ResourceGlobalSettings const& settings ) : ResourceProvider( settings ) {}
        virtual bool IsReady() const override final;

    private:

        virtual bool Initialize() override;
        virtual void RequestRawResource( ResourceRequest* pRequest ) override;
        virtual void CancelRequest( ResourceRequest* pRequest ) override;
    };
}