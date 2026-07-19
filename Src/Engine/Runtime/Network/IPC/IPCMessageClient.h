#pragma once

#include "Runtime/API.h"
#include "IPCMessage.h"
#include "../NetworkSystem.h"

//-------------------------------------------------------------------------

namespace SE::Network::IPC
{
    class SE_API_RUNTIME Client final : public ClientConnection
    {
    public:
        // Queues a message to be sent to the server. Note this is a destructive operation!! This call will move the data
        void SendMessageToServer(Message &&message);

        // Iterates over all incoming messages and calls the processing function
        void ProcessIncomingMessages(Function<void(Message const &)> messageProcessorFunction);

    private:
        virtual void ProcessMessage(void *pData, size_t size) override;
        virtual void SendMessages(Function<void(void *, uint32)> call) override;

    protected:
        List<Message> m_incomingMessages;
		List<Message> m_outgoingMessages;
    };
}