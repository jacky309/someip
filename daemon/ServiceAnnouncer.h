#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "glib.h"

#include "log.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "LocalClient.h"
#include "ServiceDiscovery.h"

#include "TCPServer.h"

/**
 * Announces the available services via UDP
 */
class ServiceAnnouncer : public ServiceRegistrationListener {

	LOG_DECLARE_CLASS_CONTEXT("SeAn", "Service announcer");

public:
	ServiceAnnouncer(Dispatcher& dispatcher, TCPServer& tcpServer) :
		m_dispatcher(dispatcher), m_timer([&]() {
							  sendServiceList();
						  }, 30000), m_tcpServer(tcpServer) {
	}

	virtual ~ServiceAnnouncer() {
	}

	void sendServiceList() {
		for ( auto service : m_dispatcher.getServices() ) {
			if ( service->isLocal() ) {
				onServiceRegistered(*service);
			}
		}
	}

	void onServiceRegistered(const Service& service) override {

		if ( service.isLocal() ) { // we don't publish remote services

			for ( auto ipAddress : TCPServer::getIPAddresses() ) {
				SomeIP::SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

				SomeIP::SomeIPServiceDiscoveryServiceOfferedEntry serviceEntry( serviceDiscoveryMessage,
												service.getServiceID(),
												SomeIP::TransportProtocol::TCP,
												ipAddress,
												m_tcpServer.getLocalTCPPort() );

				serviceDiscoveryMessage.addEntry(serviceEntry);

				ByteArray byteArray;
				NetworkSerializer s(byteArray);
				serviceDiscoveryMessage.serialize(s);

				if (sendto( m_broadcastFileDescriptor, byteArray.getData(), byteArray.size(), 0,
					    (struct sockaddr*) &addr,
					    sizeof(addr) ) < 0) {
					throw ConnectionException("Can't broadcast message");
				}

				log_info("Service published : ") << byteArray.toString();
			}
		}
	}

	void onServiceUnregistered(const Service& service) override {
		if ( service.isLocal() ) {
			for ( auto ipAddress : TCPServer::getIPAddresses() ) {

				SomeIP::SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

				SomeIP::SomeIPServiceDiscoveryServiceUnregisteredEntry serviceEntry( serviceDiscoveryMessage,
												     service.getServiceID(),
												     SomeIP::TransportProtocol::
												     TCP, ipAddress,
												     m_tcpServer.getLocalTCPPort() );

				serviceDiscoveryMessage.addEntry(serviceEntry);

				ByteArray byteArray;
				NetworkSerializer s(byteArray);
				serviceDiscoveryMessage.serialize(s);

				if (sendto( m_broadcastFileDescriptor, byteArray.getData(), byteArray.size(), 0,
					    (struct sockaddr*) &addr,
					    sizeof(addr) )
				    < 0) {
					throw ConnectionException("Can't broadcast message");
				}

				log_info("Service unpublished :") << byteArray.toString();
			}
		}
	}

	void init() {

		if ( ( m_broadcastFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 )
			throw ConnectionExceptionWithErrno("Can't create Socket");

		int broadcastEnable = 1;
		int ret =
			setsockopt( m_broadcastFileDescriptor, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable) );
		if (ret != 0)
			throw ConnectionExceptionWithErrno("Can't broadcast message");

		/* set up destination address */
		memset( &addr, 0, sizeof(addr) );
		addr.sin_family = AF_INET;
		//		addr.sin_addr.s_addr = inet_addr(SomeIP::SERVICE_DISCOVERY_BROADCAST_GROUP);
		addr.sin_addr.s_addr = INADDR_BROADCAST;
		addr.sin_port = htons(SomeIP::SERVICE_DISCOVERY_UDP_PORT);

		m_dispatcher.addServiceRegistrationListener(*this);

	}

	int m_broadcastFileDescriptor;
	struct sockaddr_in addr;

	Dispatcher& m_dispatcher;

	GLibTimer m_timer;

	TCPServer& m_tcpServer;

};
