#pragma once

#include "ServiceDiscovery.h"

#include "Dispatcher.h"
#include "TCPClient.h"
#include "TCPManager.h"

class TCPClient;

namespace SomeIP_Dispatcher {

class TCPManager : public ServiceDiscoveryListener {

	LOG_DECLARE_CLASS_CONTEXT("TCPManager", "TCPManager");

public:
	TCPManager(Dispatcher& dispatcher) :
		m_dispatcher(dispatcher), m_serviceDiscoveryDecoder(*this) {
	}

	void onRemoteClientSubscription(const SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
					const IPv4ConfigurationOption* address) {

		IPv4TCPServerIdentifier serverID(address->m_address, address->m_port);

		log_info( "New subscription from device with address %s", serverID.toString().c_str() );
	}

	void onRemoteClientSubscriptionFinished(const SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
						const IPv4ConfigurationOption* address) {
		// TODO
		assert(false);
	}

	TCPClient& getOrCreateClient(const IPv4TCPServerIdentifier& serverID);

	void onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const IPv4ConfigurationOption* address,
				      const SomeIPServiceDiscoveryMessage& message);

	void onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const IPv4ConfigurationOption* address,
					const SomeIPServiceDiscoveryMessage& message);

	ServiceDiscoveryMessageDecoder& getServiceDiscoveryMessageDecoder() {
		return m_serviceDiscoveryDecoder;
	}

	std::vector<TCPClient*> m_clients;
	Dispatcher& m_dispatcher;
	ServiceDiscoveryMessageDecoder m_serviceDiscoveryDecoder;

};

}
