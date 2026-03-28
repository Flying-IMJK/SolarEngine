#include "IPCMessageClient.h"

//-------------------------------------------------------------------------

namespace SE::Network::IPC
{
    void Client::SendMessageToServer(Message &&message)
    {
        message.m_clientConnectionID = GetClientConnectionID();
        m_outgoingMessages.Add(std::move(message));
    }

    void Client::ProcessMessage(void *pData, size_t size)
    {
        auto &message = m_incomingMessages.AddOne();
        message.Initialize(pData, size);
    }

    void Client::SendMessages(Function<void(void *, uint32)> call)
    {
        for (auto &msg : m_outgoingMessages)
        {
            call(msg.m_data.Get(), msg.m_data.Count());
        }

        m_outgoingMessages.Clear();
    }

    void Client::ProcessIncomingMessages(Function<void(Message const &)> messageProcessorFunction)
    {
        for (auto &msg : m_incomingMessages)
        {
            messageProcessorFunction(msg);
        }

        m_incomingMessages.Clear();
    }
}