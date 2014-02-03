#pragma once

#include "ServiceDiscovery.h"

class TCPClient;

#include "Dispatcher.h"
#include "TCPClient.h"
#include "TCPManager.h"

class TCPManager : public SomeIP::ServiceDiscoveryListener {

	LOG_DECLARE_CLASS_CONTEXT("TCPManager", "TCPManager");

public:
	TCPManager(Dispatcher& dispatcher) :
		m_dispatcher(dispatcher), m_serviceDiscoveryDecoder(*this) {
	}

	void onRemoteClientSubscription(const SomeIP::SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
					const SomeIP::IPv4ConfigurationOption* address) {

		IPv4TCPServerIdentifier serverID(address->m_address, address->m_port);

		log_info( "New subscription from device with address %s", serverID.toString().c_str() );
	}

	void onRemoteClientSubscriptionFinished(const SomeIP::SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
						const SomeIP::IPv4ConfigurationOption* address) {
		// TODO
		assert(false);
	}

	TCPClient& getOrCreateClient(const IPv4TCPServerIdentifier& serverID);

	void onRemoteServiceAvailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const SomeIP::IPv4ConfigurationOption* address,
				      const SomeIP::SomeIPServiceDiscoveryMessage& message);

	void onRemoteServiceUnavailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const SomeIP::IPv4ConfigurationOption* address,
					const SomeIP::SomeIPServiceDiscoveryMessage& message);

	SomeIP::ServiceDiscoveryMessageDecoder& getServiceDiscoveryMessageDecoder() {
		return m_serviceDiscoveryDecoder;
	}

	std::vector<TCPClient*> m_clients;
	Dispatcher& m_dispatcher;
	SomeIP::ServiceDiscoveryMessageDecoder m_serviceDiscoveryDecoder;

};
