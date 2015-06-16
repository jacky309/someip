#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "SomeIP-common.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "ServiceDiscovery.h"

#include "TCPServer.h"

namespace SomeIP_Dispatcher {

/**
 * Announces the available services via UDP
 */
class ServiceAnnouncer : public ServiceRegistrationListener  {

	LOG_DECLARE_CLASS_CONTEXT("SeAn", "Service announcer");

	struct AnnouncementChannel {
		int broadcastFileDescriptor;
		struct sockaddr_in addr;
		IPV4Address address;
	};

public:
	ServiceAnnouncer(Dispatcher& dispatcher, TCPManager& tcpServer, MainLoopContext& mainLoopContext) :
		m_dispatcher(dispatcher), m_timer(
			mainLoopContext.addTimeout(
				[&]() {
					announceServices();
				}, 30000) ), m_tcpServer(tcpServer) {
		m_dispatcher.addServiceRegistrationListener(*this);
	}

	virtual ~ServiceAnnouncer() {
	}

	void onServiceRegistered(const Service& service) override;

	void onServiceUnregistered(const Service& service) override;

	void announceServices() {
		for ( auto service : m_dispatcher.getServices() ) {
			if ( service->isLocal() ) {
				onServiceRegistered(*service);
			}
		}
	}

	void sendMessage(const SomeIPServiceDiscoveryMessage& serviceDiscoveryMessage, AnnouncementChannel& channel);

	SomeIPReturnCode init();

private:
	Dispatcher& m_dispatcher;

	std::unique_ptr<TimeOutMainLoopHook> m_timer;

	std::vector<AnnouncementChannel> m_channels;

	TCPManager& m_tcpServer;

};

}
