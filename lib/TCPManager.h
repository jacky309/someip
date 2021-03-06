#pragma once

#include "ServiceDiscovery.h"

#include "Dispatcher.h"
#include "TCPClient.h"
#include "TCPManager.h"
#include "ServiceAnnouncer.h"

class TCPClient;

namespace SomeIP_Dispatcher {

class TCPManager : public ServiceRegistrationListener {

	LOG_DECLARE_CLASS_CONTEXT("TCPM", "TCPManager");

public:
	TCPManager(Dispatcher& dispatcher, MainLoopContext& mainLoopContext, TCPPort port) :
		m_dispatcher(dispatcher), m_mainLoopContext(mainLoopContext) {
		m_basePort = port;
		m_dispatcher.addServiceRegistrationListener(*this);
	}

	~TCPManager() {
	}

	RemoteTCPClient* getOrCreateClient(const IPv4TCPEndPoint& serverID);

	void onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const IPv4ConfigurationOption* address,
				      const SomeIPServiceDiscoveryMessage& message);

	void onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const IPv4ConfigurationOption* address,
					const SomeIPServiceDiscoveryMessage& message);

	void onServiceRegistered(const Service& service) {
		if (service.isLocal()) {         // we don't publish remote services
			bindService(service);
		}
	}

	void onServiceUnregistered(const Service& service) {
		if (service.isLocal()) {         // we don't publish remote services
			TCPServer* server = getEndPoint(service);
			if (server!= nullptr) {
				server->removeService(service);
			}
		}
	}

	SomeIPReturnCode init(int portCount = 1) {
		return SomeIPReturnCode::OK;
	}

	void onClientReboot(RemoteTCPClient& client) {
		client.disconnect();
		client.unregisterClient();
		removeFromVector(m_clients, &client);
	}

	/**
	 * Returns the server which is publishing the given service
	 */
	TCPServer* getEndPoint(const Service& service) {
		for (auto& server : m_servers) {
			if (server->isServiceRegistered(service))
				return server;
		}

		return nullptr;
	}

	/**
	 * Returns a server which is available to publish the service given as parameter
	 */
	SomeIPReturnCode bindService(const Service& service) {

		// Search for a server which has no service currently registered with the serviceID
		TCPServer* availableServer = nullptr;
		for (auto& server : m_servers) {
			if (!server->isServiceRegistered(service.getServiceIDs().serviceID))
				availableServer = server;
		}

		if (availableServer == nullptr) {
			availableServer = new TCPServer(m_dispatcher, *this, m_basePort, m_mainLoopContext);
			auto code = availableServer->init(m_portCount);
			if (!isError(code))
				m_servers.push_back(availableServer);
		}

		if (availableServer != nullptr) {
			availableServer->addService(service);
			return SomeIPReturnCode::OK;
		} else {
			return SomeIPReturnCode::ERROR;
		}
	}

	const std::vector<IPV4Address> getIPAddresses() const {
		return TCPServer::getAllIPAddresses();
	}

private:
	std::vector<RemoteTCPClient*> m_clients;
	Dispatcher& m_dispatcher;
	MainLoopContext& m_mainLoopContext;
	std::vector<TCPServer*> m_servers;
	TCPPort m_basePort = -1;
	int m_portCount = 10;

};

}
