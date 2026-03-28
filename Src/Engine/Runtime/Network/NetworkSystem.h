#pragma once

#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/String.h"

#include "Runtime/API.h"
#include "Core/Systems.h"

//-------------------------------------------------------------------------

struct SteamNetConnectionStatusChangedCallback_t;

//-------------------------------------------------------------------------

namespace SE::Network
{
    class Network;
	class NetworkSystem;

    #define ADDRESS_STRING_SIZE 30

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME ServerConnection
    {
		friend Network;
		friend NetworkSystem;
        static void ConnectionChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo);

    public:
        struct SE_API_RUNTIME ClientInfo
        {
			ClientInfo() = default;

            ClientInfo(uint32 ID, StringAnsi const &address)
                : m_ID(ID), m_address(address)
            {
                ENGINE_ASSERT(m_ID != 0 && !m_address.IsEmpty());
            }

            uint32 m_ID;
			StringAnsi m_address;

			bool operator==(ClientInfo & clientInfo)
			{
				return m_ID == clientInfo.m_ID && m_address == clientInfo.m_address;
			}
        };
        
    public:
        ServerConnection() = default;
        ServerConnection(ServerConnection const &) = default;
        virtual ~ServerConnection();

        ServerConnection &operator=(ServerConnection const &) = delete;

        // Server Info
        //-------------------------------------------------------------------------

        inline bool IsRunning() const { return m_socketHandle != 0 && m_pollingGroupHandle != 0; }
        inline uint32 GetSocketHandle() const { return m_socketHandle; }

        // Client info
        //-------------------------------------------------------------------------

        inline int32 GetNumConnectedClients() const { return (int32)m_connectedClients.Count(); }
        inline List<ClientInfo> const &GetConnectedClients() const { return m_connectedClients; }

        inline ClientInfo const &GetConnectedClientInfo(int32 clientIdx) const
        {
            ENGINE_ASSERT(clientIdx >= 0 && clientIdx < (int32)m_connectedClients.Count());
            return m_connectedClients[clientIdx];
        }

        inline bool HasConnectedClient(uint32 clientID) const
        {
			Function<bool(const ClientInfo &)> SearchPredicate = [clientID](ClientInfo const &info)
            { 
                return info.m_ID == clientID; 
            };
            return ListExtensions::Any(m_connectedClients, SearchPredicate);
        }

        // Messages
        //-------------------------------------------------------------------------

        virtual void ProcessMessage(uint32 connectionID, void *pData, size_t size) = 0;
        virtual void SendMessages(Function<void(uint32, void*, uint32)> call) = 0;

    private:
        bool TryStartConnection(uint16 portNumber);
        void CloseConnection();
        void Update();

        void AddConnectedClient(uint32 clientID, StringAnsi const &clientAddress);
        void RemoveConnectedClient(uint32 clientID);

    protected:
        uint32 m_socketHandle = 0;
        uint32 m_pollingGroupHandle = 0;
        List<ClientInfo> m_connectedClients;
    };

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME ClientConnection
    {
        friend Network;
		friend NetworkSystem;
        static void ConnectionChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo);

    public:
        enum class Status
        {
            Disconnected,
            Connecting,
            Connected,
            Reconnecting,
            ConnectionFailed
        };

    public:
        ClientConnection() = default;
        ClientConnection(ClientConnection const &) = default;
        virtual ~ClientConnection();

        ClientConnection &operator=(ClientConnection const &) = delete;

        inline bool IsConnected() const { return m_status == Status::Connected; }
        inline bool IsConnecting() const { return m_status == Status::Connecting || m_status == Status::Reconnecting; };
        inline bool HasConnectionFailed() const { return m_status == Status::ConnectionFailed; }
        inline bool IsDisconnected() const { return m_status == Status::Disconnected; }

        inline uint32 const &GetClientConnectionID() const { return m_connectionHandle; }
        inline StringAnsi const &GetAddress() const { return m_address; }

        virtual void ProcessMessage(void *pData, size_t size) = 0;

        virtual void SendMessages(Function<void(void *, uint32)> call) = 0;

    private:
        bool TryStartConnection();
        void CloseConnection();
        void Update();

    private:
		StringAnsi m_address;
        uint32 m_connectionHandle = 0;
        uint32 m_reconnectionAttemptsRemaining = 5;
        Status m_status = Status::Disconnected;
    };

    //-------------------------------------------------------------------------

	class SE_API_RUNTIME Network
    {
		// Start a server connection on the specified port
        static bool StartServerConnection(ServerConnection *pServerConnection, uint16 portNumber);
        static void StopServerConnection(ServerConnection *pServerConnection);

        // Start a client connection to a specified address. Address format: "XXX.XXX.XXX.XXX:Port"
        static bool StartClientConnection(ClientConnection *pClientConnection, char const *pAddress);
        static void StopClientConnection(ClientConnection *pClientConnection);
    };
}