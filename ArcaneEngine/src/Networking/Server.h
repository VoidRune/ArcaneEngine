#pragma once
#include "Core/Buffer.h"

#include <functional>
#include <thread>
#include <unordered_map>

class ISteamNetworkingSockets;
struct SteamNetConnectionStatusChangedCallback_t;

namespace Arc
{
	using ClientID = uint32_t;

	class Server
	{
	public:
		using DataReceivedCallback = std::function<void(const ClientID&, const Buffer)>;
		using ClientConnectedCallback = std::function<void(const ClientID&)>;
		using ClientDisconnectedCallback = std::function<void(const ClientID&)>;
	public:
		Server();
		~Server();

		void Start(int32_t port);
		void Stop();

		void SetDataReceivedCallback(const DataReceivedCallback& callback);
		void SetClientConnectedCallback(const ClientConnectedCallback& callback);
		void SetClientDisconnectedCallback(const ClientDisconnectedCallback& callback);

		void SendBufferToClient(ClientID clientID, Buffer buffer, bool reliable = true);
		void SendBufferToAllClients(Buffer buffer, ClientID excludeClientID = 0, bool reliable = true);

	private:
		void NetworkThreadFunc(); // Server thread

		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* status);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* status);

		// Server functionality
		void PollIncomingMessages();
		void SetClientNick(ClientID hConn, const char* nick);
		void PollConnectionStateChanges();

		void OnFatalError(const std::string& message);

	private:

		std::thread m_NetworkThread;
		DataReceivedCallback m_DataReceivedCallback;
		ClientConnectedCallback m_ClientConnectedCallback;
		ClientDisconnectedCallback m_ClientDisconnectedCallback;

		int32_t m_Port = 0;
		bool m_Running = false;
		std::unordered_map<ClientID, std::string> m_ConnectedClients;

		using ListenSocket = uint32_t;
		using PollGroup = uint32_t;

		ISteamNetworkingSockets* m_Interface = nullptr;
		ListenSocket m_ListenSocket = 0u;
		PollGroup m_PollGroup = 0u;
	};

}