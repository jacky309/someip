#include "ServiceAnnouncer.h"

namespace SomeIP_Dispatcher {

void ServiceAnnouncer::onServiceRegistered(const Service& service) {

	if ( service.isLocal() ) {         // we don't publish remote services

		for ( auto ipAddress : TCPServer::getIPAddresses() ) {
			SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

			SomeIPServiceDiscoveryServiceOfferedEntry serviceEntry( serviceDiscoveryMessage,
										service.getServiceID(),
										TransportProtocol::TCP,
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

void ServiceAnnouncer::onServiceUnregistered(const Service& service) {
	if ( service.isLocal() ) {
		for ( auto ipAddress : TCPServer::getIPAddresses() ) {

			SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

			SomeIPServiceDiscoveryServiceUnregisteredEntry serviceEntry( serviceDiscoveryMessage,
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

			log_info() << "Service unpublished :" << byteArray.toString();
		}
	}
}

void ServiceAnnouncer::init() {

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
	addr.sin_port = htons(SERVICE_DISCOVERY_UDP_PORT);

	m_dispatcher.addServiceRegistrationListener(*this);

}

}
