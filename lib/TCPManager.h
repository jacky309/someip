#pragma once

#include "ServiceDiscovery.h"

#include "Dispatcher.h"
#include "TCPClient.h"
#include "TCPManager.h"
#include "ServiceAnnouncer.h"

class TCPClient;

namespace SomeIP_Dispatcher {

class TCPManager : private ServiceRegistrationListener {

	LOG_DECLARE_CLASS_CONTEXT("TCPM", "TCPManager");

public:
	TCPManager(Dispatcher& dispatcher, MainLoopContext& mainLoopContext, int port) :
		m_dispatcher(dispatcher), m_mainLoopContext(mainLoopContext), m_server(dispatcher, *this, port, mainLoopContext) {
		m_dispatcher.addServiceRegistrationListener(*this);
	}

	~TCPManager() {
		m_dispatcher.removeServiceRegistrationListener(*this);
	}

	void onServiceRegistered(const Service& service) override {
		if (service.isLocal()) {
			assert(m_namespace.count(service.getServiceIDs().serviceID) == 0);
			m_namespace[service.getServiceIDs().serviceID] = service.getServiceIDs().instanceID;
			log_debug() << m_namespace;
		}
	}

	void onServiceUnregistered(const Service& service) override {
		if (service.isLocal()) {
			assert(m_namespace.count(service.getServiceIDs().serviceID) == 1);
			m_namespace.erase(service.getServiceIDs().serviceID);
		}
	}

	TCPClient& getOrCreateClient(const IPv4TCPEndPoint& serverID);

	void onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const IPv4ConfigurationOption* address,
				      const SomeIPServiceDiscoveryMessage& message);

	void onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const IPv4ConfigurationOption* address,
					const SomeIPServiceDiscoveryMessage& message);

	ServiceInstanceNamespace& getServiceNamespace() {
		return m_namespace;
	}

	SomeIPReturnCode init(int portCount = 1) {
		return m_server.init(portCount);
	}

	const std::vector<IPv4TCPEndPoint> getIPAddresses() const {
		return m_server.getIPAddresses();
	}

private:
	std::vector<TCPClient*> m_clients;
	Dispatcher& m_dispatcher;
	MainLoopContext& m_mainLoopContext;

	TCPServer m_server;
	ServiceInstanceNamespace m_namespace;

};

}
