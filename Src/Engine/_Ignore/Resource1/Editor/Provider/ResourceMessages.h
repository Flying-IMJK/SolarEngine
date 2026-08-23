#pragma once

#include "Core/Serialization/BinarySerialization.h"
#include "Runtime/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace SGE
{
    enum class ResourceMessageID
    {
        // Client To Server
        RequestResource = 1,

        // Server To Client
        ResourceRequestComplete = 2,
        ResourceUpdated = 3,
    };

    //-------------------------------------------------------------------------

    struct NetworkResourceRequest
    {
        List<ResID > m_resourceIDs;
    };

    //-------------------------------------------------------------------------

    struct ResourceResponse
    {
		ResourceResponse() = default;

		ResourceResponse(ResID  const &ID, String const &path) : resourceID(ID), filePath(path)
		{
		}

		ResourceResponse(ResID  const &ID, String const &path, String const &log) : resourceID(ID), filePath(path), log(log)
		{
		}

		ResID resourceID;
		String filePath;
		String log;
    };

}