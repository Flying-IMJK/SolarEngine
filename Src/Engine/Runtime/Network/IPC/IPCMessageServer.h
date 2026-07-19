#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "IPCMessage.h"
#include "../NetworkSystem.h"

//-------------------------------------------------------------------------

namespace SE::Network::IPC
{
    class SE_API_RUNTIME Server final : public ServerConnection
    {

    public:
        // Queues a message to be sent. Note this is a destructive operation!! This call will move the data
        void SendNetworkMessage(Message &&message);

        // Iterates over all incoming messages and calls the processing function
        void ProcessIncomingMessages(Function<void(Message const &)> messageProcessorFunction);

    private:
        virtual void ProcessMessage(uint32 connectionID, void *pData, uint64 size) override;
        virtual void SendMessages(Function<void(uint32, void*, uint32)> call) override;

    private:
        List<Message> m_incomingMessages;
		List<Message> m_outgoingMessages;
    };
}
