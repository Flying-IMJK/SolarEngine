#pragma once

#include "Runtime/API.h"
#include "Core/Types/Variable.h"

namespace SGE
{
	class ResID;

	class SE_API_RUNTIME ResourceRequesterID
	{
	public:
		static constexpr uint64 s_manualRequestID = 0;
		static constexpr uint64 s_toolsRequestID = 0xFFFFFFFFFFFFFFFF;

	public:
		// No ID - manual request
		ResourceRequesterID() = default;

		// Install dependency reference
		ResourceRequesterID(ResID const& resourceID);

		// Explicit ID - generally refers to an entity or tools ID
/*		explicit ResourceRequesterID(uint64 ID) : m_ID(ID)
		{
			ENGINE_ASSERT(ID > 0);
		}*/

		//-------------------------------------------------------------------------

/*		// This ID refers to a manual request outside of the default resource loading flow (usually only used for resources like maps)
		inline bool IsManualRequest() const
		{
			return m_ID == s_manualRequestID;
		}

		// This ID refers to a request originating from the tools, this allows for the tools to reload resources that they are editing
		inline bool IsToolsRequest() const
		{
			return m_ID == s_toolsRequestID;
		}

		// A normal request via the entity system
		inline bool IsNormalRequest() const
		{
			return m_ID > 0 && !m_isInstallDependency;
		}*/

		// A install dependency request, coming from the resource system as part of resource loading
		inline bool IsInstallDependencyRequest() const
		{
			return m_isInstallDependency;
		}

		//-------------------------------------------------------------------------

		// Get the requester ID
		inline ResID GetID() const
		{
			return m_ID;
		}

		// Get the ID for the data path for install dependencies, used for reverse look ups
		inline ResID GetInstallDependencyResourcePathID() const
		{
			ENGINE_ASSERT(m_isInstallDependency);
			return m_ID;
		}

		//-------------------------------------------------------------------------

		inline bool operator==(ResourceRequesterID const& rhs) const
		{
			return m_ID == rhs.m_ID;
		}
		inline bool operator!=(ResourceRequesterID const& rhs) const
		{
			return m_ID != rhs.m_ID;
		}

	private:
		ResID m_ID;
		bool m_isInstallDependency = false;
	};

	inline uint32 GetHash(const ResourceRequesterID& requesterId)
	{
		return GetHash(requesterId.GetID());
	}
}
