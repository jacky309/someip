#pragma once

#include "ServiceDiscovery.h"

#include "Dispatcher.h"
#include "TCPClient.h"
#include "TCPManager.h"
#include "ServiceAnnouncer.h"

class TCPClient;

namespace SomeIP_Dispatcher {

class TCPManager {

	LOG_DECLARE_CLASS_CONTEXT("TCPM", "TCPManager");

public:
	TCPManager(Dispatcher& dispatcher) :
		m_dispatcher(dispatcher) {
	}

	TCPClient& getOrCreateClient(const IPv4TCPServerIdentifier& serverID);

	void onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const IPv4ConfigurationOption* address,
				      const SomeIPServiceDiscoveryMessage& message);

	void onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const IPv4ConfigurationOption* address,
					const SomeIPServiceDiscoveryMessage& message);


private:
	std::vector<TCPClient*> m_clients;
	Dispatcher& m_dispatcher;

};

}
