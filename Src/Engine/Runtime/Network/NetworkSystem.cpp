#include "NetworkSystem.h"

#include "Core/Logging/Logging.h"
#include "Core/Memory/Memory.h"
#include "Core/Thread/Threading.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Platform/Platform.h"
#include "Core/Types/Strings/String.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

//-------------------------------------------------------------------------

namespace SE::Network
{
	static void NetworkDebugOutputFunction(ESteamNetworkingSocketsDebugOutputType type, char const *pMessage)
	{
		LOG_INFO("NetWork", "{0}", StringAnsiView(pMessage));

		// TODO: initialize rpmalloc for the steam network thread
		if (type == k_ESteamNetworkingSocketsDebugOutputType_Bug)
		{
			//LOG_FATAL_ERROR( "Network", "({0:10.6f}) {1}", currentTime * 1e-6, pMessage);
		}
		else if (type == k_ESteamNetworkingSocketsDebugOutputType_Error)
		{
			//LOG_ERROR( "Network", "({0:10.6f}) {1}", currentTime * 1e-6, pMessage );
		}
		else if (type == k_ESteamNetworkingSocketsDebugOutputType_Warning)
		{
			//LOG_WARNING( "Network", "({0:10.6f}) {1}", currentTime * 1e-6, pMessage );
		}
		else if (type == k_ESteamNetworkingSocketsDebugOutputType_Important)
		{
			//LOG_INFO( "Network", "({0:10.6f}) {1}", currentTime * 1e-6, pMessage );
		}
		else // Verbose
		{
			//LOG_INFO( "Network", "({0:10.6f}) {1}s", currentTime * 1e-6, pMessage );
		}
	}

	class NetworkSystem final : public ISystem
	{
		friend struct NetworkCallbackHandler;
		ENGINE_SYSTEM(NetworkSystem)
	public:
		NetworkSystem() : ISystem(SE_TEXT("NetWork"))
		{
		};

		int64 m_logTimeZero = 0;
		List<ServerConnection *> m_serverConnections;
		List<ClientConnection *> m_clientConnections;

	private:
		bool OnInit() override
		{
			return false;
			SteamDatagramErrMsg errMsg;
			if (!GameNetworkingSockets_Init(nullptr, errMsg))
			{
				LOG_ERROR("Network", "Failed to initialize network system: {0}", String(errMsg));
				return false;
			}

			//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
			SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg, NetworkDebugOutputFunction);
#endif

			m_logTimeZero = SteamNetworkingUtils()->GetLocalTimestamp();
			return true;

			return ISystem::OnInit();
		}

		void OnDispose() override
		{
			return;
			// Give connections time to finish up. This is an application layer protocol here, it's not TCP.  Note that if you have an application and you need to be
			// more sure about cleanup, you won't be able to do this.  You will need to send a message and then either wait for the peer to close the connection, or
			// you can pool the connection to see if any reliable data is pending.
			Threading::Sleep(250);

			GameNetworkingSockets_Kill();
		}

		void OnUpdate() override
		{
			return;
			auto pInterface = SteamNetworkingSockets();
			ENGINE_ASSERT(pInterface != nullptr);

			//-------------------------------------------------------------------------
			// Call all connection state callbacks
			//-------------------------------------------------------------------------

			pInterface->RunCallbacks();

			//-------------------------------------------------------------------------
			// Servers
			//-------------------------------------------------------------------------

			for (auto pServerConnection : m_serverConnections)
			{
				pServerConnection->Update();
			}

			//-------------------------------------------------------------------------
			// Clients
			//-------------------------------------------------------------------------

			for (auto pClientConnection : m_clientConnections)
			{
				pClientConnection->Update();
			}
		}
	};

	ENGINE_SYSTEM_REGISTER(NetworkSystem)


    //-------------------------------------------------------------------------

    ServerConnection::~ServerConnection()
    {
        ENGINE_ASSERT(m_pollingGroupHandle == k_HSteamNetPollGroup_Invalid && m_socketHandle == k_HSteamListenSocket_Invalid);
    }

    void ServerConnection::AddConnectedClient(uint32 clientHandle, StringAnsi const &clientAddress)
    {
        ENGINE_ASSERT(!HasConnectedClient(clientHandle));
        m_connectedClients.Add(ClientInfo(clientHandle, clientAddress));
    }

    void ServerConnection::RemoveConnectedClient(uint32 clientHandle)
    {
        Function<bool(const ClientInfo &)> SearchPredicate = [clientHandle](const ClientInfo &info)
        { 
            return info.m_ID == clientHandle; 
        };
        auto index = ListExtensions::IndexOf(m_connectedClients, SearchPredicate);
        ENGINE_ASSERT(index != INVALID_INDEX);
        m_connectedClients.RemoveAt(index);
    }

    bool ServerConnection::TryStartConnection(uint16_t portNumber)
    {
        ISteamNetworkingSockets *pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        //-------------------------------------------------------------------------

        SteamNetworkingIPAddr serverLocalAddr;
        serverLocalAddr.Clear();
        serverLocalAddr.m_port = portNumber;

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)ConnectionChangedCallback);
        m_socketHandle = pInterface->CreateListenSocketIP(serverLocalAddr, 1, &opt);
        if (m_socketHandle == k_HSteamListenSocket_Invalid)
        {
            LOG_ERROR("Network", "Server Connection Failed to listen on port {0}", portNumber);
            return false;
        }

        m_pollingGroupHandle = pInterface->CreatePollGroup();
        if (m_pollingGroupHandle == k_HSteamNetPollGroup_Invalid)
        {
            LOG_ERROR("Network", "Server Connection Failed to listen on port {0}", portNumber);
            return false;
        }

        return true;
    }

    void ServerConnection::CloseConnection()
    {
        ISteamNetworkingSockets *pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        //-------------------------------------------------------------------------

        for (auto const &clientInfo : m_connectedClients)
        {
            pInterface->CloseConnection(clientInfo.m_ID, 0, "Server Dispose", true);
        }
        m_connectedClients.Clear();

        //-------------------------------------------------------------------------

        if (m_pollingGroupHandle != k_HSteamNetPollGroup_Invalid)
        {
            pInterface->DestroyPollGroup(m_pollingGroupHandle);
            m_pollingGroupHandle = k_HSteamNetPollGroup_Invalid;
        }

        if (m_socketHandle != k_HSteamListenSocket_Invalid)
        {
            pInterface->CloseListenSocket(m_socketHandle);
            m_socketHandle = k_HSteamListenSocket_Invalid;
        }
    }

    void ServerConnection::Update()
    {
        ISteamNetworkingSockets* pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        // Receive
        //-------------------------------------------------------------------------

        ISteamNetworkingMessage *pIncomingMsg = nullptr;
        int32_t const numMsgs = pInterface->ReceiveMessagesOnPollGroup(m_pollingGroupHandle, &pIncomingMsg, 1);

        if (numMsgs < 0)
        {
            LOG_ERROR("Network", "Server Connection Failed to check for messages!Something has gone wrong with the server connection!");
            return;
        }

        if (numMsgs > 0)
        {
            ProcessMessage(pIncomingMsg->m_conn, pIncomingMsg->m_pData, pIncomingMsg->m_cbSize);
            pIncomingMsg->Release();
        }

        // Send
        //-------------------------------------------------------------------------
        SendMessages([pInterface](uint32 connectionHandle, void *pData, uint32 size)
		{
		  pInterface->SendMessageToConnection(connectionHandle, pData, size, k_nSteamNetworkingSend_Reliable, nullptr);
		});
    }

    void ServerConnection::ConnectionChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
    {
		NetworkSystem *const networkSystem = Systems::GetSystem<NetworkSystem>();
		ENGINE_ASSERT(networkSystem);

        auto pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        // Find Server Connection
        //-------------------------------------------------------------------------

        auto FindServerConnection = [pInfo](const ServerConnection *pConnection)
        {
            return pConnection->GetSocketHandle() == pInfo->m_info.m_hListenSocket;
        };

		Function<bool(ServerConnection* const&)> predicate = [pInfo](ServerConnection* const &pConnection)
		{
		  return pConnection->GetSocketHandle() == pInfo->m_info.m_hListenSocket;
		};

        int index = ListExtensions::IndexOf(networkSystem->m_serverConnections, predicate);

        ENGINE_ASSERT(index != INVALID_INDEX);
        ServerConnection *pServerConnection = networkSystem->m_serverConnections[index];

        // Handle state change
        //-------------------------------------------------------------------------

        switch (pInfo->m_info.m_eState)
        {
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            // Ignore if they were not previously connected.  (If they disconnected before we accepted the connection.)
            if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
            {
                // Note that clients should have been found, because this is the only code path where we remove clients( except on shutdown ), and connection change callbacks are dispatched in queue order.
                ENGINE_ASSERT(pServerConnection->HasConnectedClient(pInfo->m_hConn));
                pServerConnection->RemoveConnectedClient(pInfo->m_hConn);
            }
            else
            {
                ENGINE_ASSERT(pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
            }

            // Clean up the connection.  This is important!
            // The connection is "closed" in the network sense, but
            // it has not been destroyed.  We must close it on our end, too
            // to finish up.  The reason information do not matter in this case,
            // and we cannot linger because it's already closed on the other end,
            // so we just pass 0's.
            pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
            break;
        }

        case k_ESteamNetworkingConnectionState_Connecting:
        {
            // This must be a new connection
            ENGINE_ASSERT(!pServerConnection->HasConnectedClient(pInfo->m_hConn));

            // A client is attempting to connect
            // Try to accept the connection.
            if (pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
            {
                // This could fail. If the remote host tried to connect, but then
                // disconnected, the connection may already be half closed.  Just
                // destroy whatever we have on our side.
                pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
                break;
            }

            // Assign the poll group
            if (!pInterface->SetConnectionPollGroup(pInfo->m_hConn, pServerConnection->m_pollingGroupHandle))
            {
                pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
                break;
            }

            StringAnsi clientAddress;
			clientAddress.Resize(ADDRESS_STRING_SIZE);
            pInfo->m_info.m_addrRemote.ToString(clientAddress.Get(), ADDRESS_STRING_SIZE, true);
            pServerConnection->AddConnectedClient(pInfo->m_hConn, clientAddress);
            break;
        }

        case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            // We will get a callback immediately after accepting the connection.
            // Since we are the server, we can ignore this, it's not news to us.
            break;

        default:
            // Silences -Wswitch
            break;
        }
    }

    //-------------------------------------------------------------------------

    ClientConnection::~ClientConnection()
    {
        ENGINE_ASSERT(m_connectionHandle == k_HSteamNetConnection_Invalid);
    }

    bool ClientConnection::TryStartConnection()
    {
        SteamNetworkingIPAddr serverAddr;
        if (!serverAddr.ParseString(m_address.Get()))
        {
            LOG_ERROR("Network", "Client Connection Invalid client IP address provided: {0}", m_address);
            return false;
        }

        if (serverAddr.m_port == 0)
        {
            LOG_ERROR("Network", "Client Connection No port for client address provided: {0}", m_address);
            return false;
        }

        //-------------------------------------------------------------------------

        ISteamNetworkingSockets *pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)ConnectionChangedCallback);
        m_connectionHandle = pInterface->ConnectByIPAddress(serverAddr, 1, &opt);
        if (m_connectionHandle == k_HSteamNetConnection_Invalid)
        {
            LOG_ERROR("Network", "Client Connection Failed to create connection");
            return false;
        }

        m_status = Status::Connecting;
        return true;
    }

    void ClientConnection::CloseConnection()
    {
        ISteamNetworkingSockets *pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        if (m_connectionHandle != k_HSteamNetConnection_Invalid)
        {
            pInterface->CloseConnection(m_connectionHandle, k_ESteamNetConnectionEnd_App_Generic, "Closing Connection", false);
            m_connectionHandle = k_HSteamNetConnection_Invalid;
        }

        m_status = Status::Disconnected;
    }

    void ClientConnection::Update()
    {
        ISteamNetworkingSockets *pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        // Handle Reconnection Attempts
        //-------------------------------------------------------------------------

        if (m_status == Status::Reconnecting)
        {
            if (m_reconnectionAttemptsRemaining > 0)
            {
                m_reconnectionAttemptsRemaining--;
                Threading::Sleep(100);
                if (!TryStartConnection())
                {
                    m_status = Status::Reconnecting;
                    return;
                }
            }
            else
            {
                m_status = Status::ConnectionFailed;
                return;
            }
        }

        // Run Client Update
        //-------------------------------------------------------------------------

        if (IsConnected())
        {
            // Receive
            //-------------------------------------------------------------------------

            ISteamNetworkingMessage *pIncomingMsg = nullptr;
            int32_t const numMsgs = pInterface->ReceiveMessagesOnConnection(m_connectionHandle, &pIncomingMsg, 1);

            // Handle invalid connection handle
            if (numMsgs < 0)
            {
                LOG_FATAL("Network", "Client Connection Client connection handle is invalid, we've likely lost connection to the server");
                return;
            }

            // Process received messages
            if (numMsgs > 0)
            {
                ProcessMessage(pIncomingMsg->m_pData, pIncomingMsg->m_cbSize);
                pIncomingMsg->Release();
            }

            // Send
            //-------------------------------------------------------------------------
            SendMessages([pInterface, this](void *pData, uint32 size)
			{
			  pInterface->SendMessageToConnection(m_connectionHandle, pData, size, k_nSteamNetworkingSend_Reliable, nullptr);
			});
        }
    }

    void ClientConnection::ConnectionChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
    {
		NetworkSystem *const networkSystem = Systems::GetSystem<NetworkSystem>();
		ENGINE_ASSERT(networkSystem);

        auto pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        // Find Client Connection
        //-------------------------------------------------------------------------
        Function<bool(ClientConnection * const&)> FindClientConnection = [pInfo](ClientConnection const *pConnection)
        {
            return pConnection->GetClientConnectionID() == pInfo->m_hConn;
        };

        auto index = ListExtensions::IndexOf(networkSystem->m_clientConnections, FindClientConnection);

        // Ignore status changes messages for already closed connections
        if (index == INVALID_INDEX)
        {
            return;
        }

        ClientConnection *pClientConnection = networkSystem->m_clientConnections[index];

        // Handle state change
        //-------------------------------------------------------------------------

        // What's the state of the connection?
        switch (pInfo->m_info.m_eState)
        {
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            // Clean up the connection.  This is important!
            // The connection is "closed" in the network sense, but
            // it has not been destroyed.  We must close it on our end, too
            // to finish up.  The reason information do not matter in this case,
            // and we cannot linger because it's already closed on the other end,
            // so we just pass 0's.
            pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
            pClientConnection->m_connectionHandle = k_HSteamNetConnection_Invalid;

            // Try restore connection
            pClientConnection->m_status = ClientConnection::Status::Reconnecting;
            break;
        }

        case k_ESteamNetworkingConnectionState_Connecting:
        case k_ESteamNetworkingConnectionState_FindingRoute:
        {
            pClientConnection->m_status = Status::Connecting;
        }
        break;

        case k_ESteamNetworkingConnectionState_Connected:
        {
            pClientConnection->m_reconnectionAttemptsRemaining = 5;
            pClientConnection->m_status = Status::Connected;
        }
        break;

        case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
            break;

        default:
            // Silences -Wswitch
            break;
        }
    }

    //-------------------------------------------------------------------------

    bool Network::StartServerConnection(ServerConnection *pServerConnection, uint16_t portNumber)
    {
		NetworkSystem *const networkSystem = Systems::GetSystem<NetworkSystem>();
		ENGINE_ASSERT(networkSystem);

        ENGINE_ASSERT(pServerConnection != nullptr && !pServerConnection->IsRunning());
        if (pServerConnection->TryStartConnection(portNumber))
        {
			networkSystem->m_serverConnections.Add(pServerConnection);
            return true;
        }

        return false;
    }

    void Network::StopServerConnection(ServerConnection *pServerConnection)
    {
		NetworkSystem *const networkSystem = Systems::GetSystem<NetworkSystem>();
		ENGINE_ASSERT(networkSystem);

        ENGINE_ASSERT(pServerConnection != nullptr);
        pServerConnection->CloseConnection();
		int index = networkSystem->m_serverConnections.FindFirst(pServerConnection);
		if (index != INVALID_INDEX)
		{
			networkSystem->m_serverConnections.RemoveAt(index);
		}
    }

    //-------------------------------------------------------------------------

    bool Network::StartClientConnection(ClientConnection *pClientConnection, char const *pAddress)
    {
		NetworkSystem *const networkSystem = Systems::GetSystem<NetworkSystem>();

		ENGINE_ASSERT(networkSystem);
        ENGINE_ASSERT(pClientConnection != nullptr && pClientConnection->IsDisconnected());
        ENGINE_ASSERT(pAddress != nullptr);

        ISteamNetworkingSockets *pInterface = SteamNetworkingSockets();
        ENGINE_ASSERT(pInterface != nullptr);

        //-------------------------------------------------------------------------

        pClientConnection->m_address = pAddress;
        if (!pClientConnection->TryStartConnection())
        {
            return false;
        }

		networkSystem->m_clientConnections.Add(pClientConnection);
        return true;
    }

    void Network::StopClientConnection(ClientConnection *pClientConnection)
    {
		NetworkSystem *const networkSystem = Systems::GetSystem<NetworkSystem>();
		ENGINE_ASSERT(networkSystem);
        ENGINE_ASSERT(pClientConnection != nullptr && !pClientConnection->IsDisconnected());

        pClientConnection->CloseConnection();
        pClientConnection->m_address.Clear();
		int index = networkSystem->m_clientConnections.FindFirst(pClientConnection);
		if (index != INVALID_INDEX)
		{
			networkSystem->m_clientConnections.RemoveAt(index);
		}
    }

}