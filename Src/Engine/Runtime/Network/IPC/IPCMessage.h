#pragma once
#include "Runtime/API.h"
#include "Core/Types/Collections/List.h"
//#include "Core/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace SE::Network::IPC
{
    enum class MessageType
    {
        Generic = 0,
        NumMessageTypes
    };

    //-------------------------------------------------------------------------

    typedef int32 MessageID;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME Message
    {
        friend class Server;
        friend class Client;
        constexpr static MessageID const InvalidID = 0xFFFFFFFF;

    public:
        Message() { Initialize(InvalidID); }
        explicit Message(MessageID ID) { Initialize(ID); }
        explicit Message(MessageID ID, void *pData, uint64 dataSize) { Initialize(ID, pData, dataSize); }

        explicit Message(Message &&msg)
        {
            m_clientConnectionID = msg.m_clientConnectionID;
            m_data.Swap(msg.m_data);
        }
        explicit Message(Message const &msg)
        {
            m_clientConnectionID = msg.m_clientConnectionID;
            m_data = msg.m_data;
        }

        template <typename T>
        Message(MessageID ID, T &&typeToSerialize)
        {
            Serialization::BinaryOutputArchive archive;
            archive << typeToSerialize;
            Initialize(ID, (void *)archive.GetBinaryData(), archive.GetBinaryDataSize());
        }

        //-------------------------------------------------------------------------

        Message &operator=(Message const &msg)
        {
            m_data = msg.m_data;
            return *this;
        }

        Message &operator=(Message &&msg) noexcept
        {
            m_data.Swap(msg.m_data);
            return *this;
        }

        //-------------------------------------------------------------------------

        inline uint32 const &GetClientConnectionID() const { return m_clientConnectionID; }
        inline void SetClientConnectionID(uint32 clientConnectionID) { m_clientConnectionID = clientConnectionID; }
        inline MessageID GetMessageID() const { return *(MessageID *)m_data.Get(); }

        inline bool IsValid() const { return GetMessageID() != InvalidID; }
        inline bool HasPayload() const { return GetPayloadDataSize() > sizeof(MessageID); }
        inline uint8 const *GetPayloadData() const { return m_data.Get() + sizeof(MessageID); }
        inline uint64 GetPayloadDataSize() const { return m_data.Count() - sizeof(MessageID); }

        // Serialization functions
        template <typename T>
        inline void SetData(MessageID msgID, T const &typeToSerialize)
        {
            Serialization::BinaryOutputArchive archive;
            archive << typeToSerialize;
            Initialize(msgID, (void *)archive.GetBinaryData(), archive.GetBinaryDataSize());
        }

        // Serialization functions
        template <typename T>
        inline T GetData() const
        {
            T outType;
            Serialization::BinaryInputArchive archive;
            archive.ReadFromData(GetPayloadData(), GetPayloadDataSize());
            archive << outType;
            return outType;
        }

    private:
        void Initialize(MessageID messageID);
        void Initialize(void *pData, uint64 dataSize);
        void Initialize(MessageID messageID, void *pData, uint64 dataSize);

    private:
        List<uint8> m_data;
        uint32 m_clientConnectionID = 0;
    };
}