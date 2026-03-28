#include "IPCMessage.h"

//-------------------------------------------------------------------------

namespace SE::Network::IPC
{
    void Message::Initialize(MessageID messageID)
    {
        m_data.Resize(sizeof(MessageID));
        *(MessageID *)m_data.Get() = messageID;
    }

    void Message::Initialize(void *pData, uint64 dataSize)
    {
        ENGINE_ASSERT(pData != nullptr && dataSize >= sizeof(MessageID));
        m_data.Resize(dataSize);
        Platform::MemoryCopy(m_data.Get(), pData, dataSize);
        ENGINE_ASSERT(GetMessageID() != InvalidID);
    }

    void Message::Initialize(MessageID messageID, void *pData, uint64 dataSize)
    {
        ENGINE_ASSERT(messageID != InvalidID && pData != nullptr && dataSize > 0);
        m_data.Resize(dataSize + sizeof(MessageID));
        *(MessageID *)m_data.Get() = messageID;
		Platform::MemoryCopy(m_data.Get() + sizeof(MessageID), pData, dataSize);
    }
}