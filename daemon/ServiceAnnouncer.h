#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "glib.h"

#include "SomeIP-common.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "LocalClient.h"
#include "ServiceDiscovery.h"

#include "TCPServer.h"

namespace SomeIP_Dispatcher {

/**
 * Announces the available services via UDP
 */
class ServiceAnnouncer : public ServiceRegistrationListener {

	LOG_DECLARE_CLASS_CONTEXT("SeAn", "Service announcer");

public:
	ServiceAnnouncer(Dispatcher& dispatcher, TCPServer& tcpServer, MainLoopContext& mainLoopContext) :
		m_dispatcher(dispatcher), m_timer([&]() {
							  announceServices();
						  }, 300000, mainLoopContext), m_tcpServer(tcpServer) {
	}

	virtual ~ServiceAnnouncer() {
	}

	void announceServices() {
		for ( auto service : m_dispatcher.getServices() ) {
			if ( service->isLocal() ) {
				onServiceRegistered(*service);
			}
		}
	}

	void onServiceRegistered(const Service& service);

	void onServiceUnregistered(const Service& service);

	void sendMessage(const SomeIPServiceDiscoveryMessage& serviceDiscoveryMessage);

	void init();

private:
	int m_broadcastFileDescriptor;
	struct sockaddr_in addr;

	Dispatcher& m_dispatcher;

	GLibTimer m_timer;

	TCPServer& m_tcpServer;

};

}
