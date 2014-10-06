#include "ServiceAnnouncer.h"

namespace SomeIP_Dispatcher {

void ServiceAnnouncer::sendMessage(const SomeIPServiceDiscoveryMessage& serviceDiscoveryMessage) {

	ByteArray byteArray;
	NetworkSerializer s(byteArray);
	serviceDiscoveryMessage.serialize(s);

	if (sendto( m_broadcastFileDescriptor, byteArray.getData(), byteArray.size(), 0,
		    (struct sockaddr*) &addr,
		    sizeof(addr) ) < 0) {
		log_error() << "Can't send service discovery message : " << byteArray;
	} else {
		log_info() << "Service discovery message sent : " << byteArray;
	}


}

void ServiceAnnouncer::onServiceRegistered(const Service& service) {

	if ( service.isLocal() ) {         // we don't publish remote services

		for ( auto ipAddress : m_tcpServer.getIPAddresses() ) {
			SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

			SomeIPServiceDiscoveryServiceOfferedEntry serviceEntry(serviceDiscoveryMessage,
									       service.getServiceID(),
									       TransportProtocol::TCP,
									       ipAddress.m_address,
									       ipAddress.m_port);

			serviceDiscoveryMessage.addEntry(serviceEntry);

			sendMessage(serviceDiscoveryMessage);

		}
	}
}


void ServiceAnnouncer::onServiceUnregistered(const Service& service) {
	if ( service.isLocal() ) {
		for ( auto ipAddress : m_tcpServer.getIPAddresses() ) {

			SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

			SomeIPServiceDiscoveryServiceUnregisteredEntry serviceEntry(serviceDiscoveryMessage,
										    service.getServiceID(),
										    SomeIP::TransportProtocol::
										    TCP, ipAddress.m_address,
										    ipAddress.m_port);

			serviceDiscoveryMessage.addEntry(serviceEntry);

			sendMessage(serviceDiscoveryMessage);

		}
	}
}

SomeIPReturnCode ServiceAnnouncer::init() {

	if ( ( m_broadcastFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 )
		return SomeIPReturnCode::ERROR;

	int broadcastEnabled = 1;
	int ret =
		setsockopt( m_broadcastFileDescriptor, SOL_SOCKET, SO_BROADCAST, &broadcastEnabled, sizeof(broadcastEnabled) );
	if (ret != 0)
		throw ConnectionExceptionWithErrno("Can't broadcast message");

	/* set up destination address */
	memset( &addr, 0, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	addr.sin_port = htons(SERVICE_DISCOVERY_UDP_PORT);

	m_dispatcher.addServiceRegistrationListener(*this);

	// send a query to know about services provided by other devices;
	SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);
	SomeIPServiceDiscoveryServiceQueryEntry entry(0xFFFF);
	serviceDiscoveryMessage.addEntry(entry);
	sendMessage(serviceDiscoveryMessage);

	return SomeIPReturnCode::OK;
}

}
