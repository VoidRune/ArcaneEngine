#pragma once
#include "Core/Buffer.h"

#include <functional>
#include <thread>
#include <map>

class ISteamNetworkingSockets;
struct SteamNetConnectionStatusChangedCallback_t;

namespace Arc
{
	class Client
	{
	public:
		using DataReceivedCallback = std::function<void(const Buffer)>;
		using ServerConnectedCallback = std::function<void()>;
		using ServerDisconnectedCallback = std::function<void()>;
	public:
		Client();
		~Client();

		void ConnectToServer(const std::string& serverAddress);
		void Disconnect();

		void SetDataReceivedCallback(const DataReceivedCallback& callback);
		void SetServerConnectedCallback(const ServerConnectedCallback& callback);
		void SetServerDisconnectedCallback(const ServerDisconnectedCallback& callback);

		void SendBuffer(Buffer buffer, bool reliable = true);

	private:
		void NetworkThreadFunc();

		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* status);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* status);

		void PollIncomingMessages();
		void PollConnectionStateChanges();

	private:

		std::thread m_NetworkThread;
		DataReceivedCallback m_DataReceivedCallback;
		ServerConnectedCallback m_ServerConnectedCallback;
		ServerDisconnectedCallback m_ServerDisconnectedCallback;

		std::string m_ServerAddress;
		bool m_Running = false;

		using NetConnection = uint32_t;

		ISteamNetworkingSockets* m_Interface = nullptr;
		NetConnection m_Connection = 0u;
	};

}