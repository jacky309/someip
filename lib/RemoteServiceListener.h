#pragma once

#include "SomeIP-common.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "ServiceDiscovery.h"
#include "TCPClient.h"
#include "TCPManager.h"

#include "ServiceAnnouncer.h"

namespace SomeIP_Dispatcher {

/**
 * That class handles the registration of remote services
 */
class RemoteServiceListener : public ServiceDiscoveryListener {

	LOG_DECLARE_CLASS_CONTEXT("RSL", "Remote service listener");

public:
	RemoteServiceListener(Dispatcher& dispatcher, TCPManager& tcpManager, ServiceAnnouncer& serviceAnnouncer,
			      MainLoopInterface& mainLoopContext) :
		m_serviceDiscoveryDecoder(*this), m_tcpManager(tcpManager), m_serviceAnnouncer(serviceAnnouncer),
		m_mainLoopContext(mainLoopContext) {
	}

	~RemoteServiceListener() {
	}

	SomeIPReturnCode init();

	void onFindServiceRequested(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				    const SomeIPServiceDiscoveryMessage& message) {
		m_serviceAnnouncer.announceServices();
	}

	void onRemoteClientSubscription(const SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
					const IPv4ConfigurationOption* address) override {

		IPv4TCPEndPoint serverID(address->m_address, address->m_port);
		log_info() << "New subscription from device with address" << serverID.toString();

		// TODO : implement
	}

	void onRemoteClientSubscriptionFinished(const SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
						const IPv4ConfigurationOption* address) override {
		// TODO
		assert(false);
	}

	void onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const IPv4ConfigurationOption* address,
				      const SomeIPServiceDiscoveryMessage& message) override {
		m_tcpManager.onRemoteServiceAvailable(serviceEntry, address, message);
	}

	void onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const IPv4ConfigurationOption* address,
					const SomeIPServiceDiscoveryMessage& message) override {
		m_tcpManager.onRemoteServiceUnavailable(serviceEntry, address, message);
	}

private:
	void handleMessage();

	ServiceDiscoveryMessageDecoder m_serviceDiscoveryDecoder;

	int m_broadcastFileDescriptor = -1;

	TCPManager& m_tcpManager;
	ServiceAnnouncer& m_serviceAnnouncer;
	MainLoopInterface& m_mainLoopContext;

	std::unique_ptr<WatchMainLoopHook> m_inputDataWatcher;

};

}
