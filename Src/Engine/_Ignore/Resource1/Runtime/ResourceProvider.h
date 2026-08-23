#pragma once


#include "ResourceID.h"
#include "Runtime/Settings/GlobalSettings_Resource.h"

//-------------------------------------------------------------------------

namespace SGE
{
    //-------------------------------------------------------------------------
    // The resource provider is the system that is responsible for resolving a resource request to raw resource data
    // It is responsible for loading the request resource from the disk/network as well decompressing it
    // The raw decompressed data is then provided to the resource system for installation
    //-------------------------------------------------------------------------

    class ResourceRequest;

    //-------------------------------------------------------------------------
	/**
	 * 资源供应负责将资源请求解析为原始资源数据的系统， 它负责从磁盘或者网络加载请求的资源以及对其进行解压缩，将解压缩数据提供给资源系统
	 */
    class SE_API_RUNTIME ResourceProvider
    {

    public:
        ResourceProvider(ResourceGlobalSettings const &settings) : m_settings(settings) {}
        ResourceProvider(ResourceProvider const &) = delete;
        virtual ~ResourceProvider() {}

        virtual bool IsReady() const = 0;
        virtual bool Initialize() = 0;
        virtual void Shutdown() {}

        // Get general resource settings
        ResourceGlobalSettings const &GetSettings() const { return m_settings; }

        virtual void Update(){};

        // 请求加载资源
        virtual void RequestRawResource(ResourceRequest *pRequest) = 0;

        // 取消加载请求
        virtual void CancelRequest(ResourceRequest *pRequest) = 0;

        // Get any externally updated resource for this update
        #ifdef SE_DEVELOPMENT
        virtual List<ResID > const &GetExternallyUpdatedResources() const
        {
            static List<ResID > const emptyVector;
            return emptyVector;
        }
        #endif

    protected:
        ResourceGlobalSettings const m_settings;
    };
}