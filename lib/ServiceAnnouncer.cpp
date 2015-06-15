#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ServiceAnnouncer.h"
#include "TCPManager.h"

namespace SomeIP_Dispatcher {

void ServiceAnnouncer::sendMessage(const SomeIPServiceDiscoveryMessage& serviceDiscoveryMessage,
		AnnouncementChannel& channel) {

	ByteArray byteArray;
	NetworkSerializer s(byteArray);
	serviceDiscoveryMessage.serialize(s);

	if (sendto(channel.broadcastFileDescriptor, byteArray.getData(), byteArray.size(), 0,
			(struct sockaddr*) &channel.addr, sizeof(channel.addr)) < 0) {
		log_error() << "Can't send service discovery message : " << byteArray;
	} else {
		log_info() << "Service discovery message sent : " << byteArray;
	}

}

void ServiceAnnouncer::onServiceRegistered(const Service& service) {

	if (service.isLocal()) {         // we don't publish remote services

		TCPServer* server = m_tcpServer.bindService(service);

		assert(server != nullptr);

		for (auto channel : m_channels)
			for (auto ipAddress : server->getIPAddresses())
				if (channel.address == ipAddress.getAddress()) {
					SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

					SomeIPServiceDiscoveryServiceOfferedEntry serviceEntry(serviceDiscoveryMessage,
							service.getServiceIDs(), TransportProtocol::TCP, ipAddress.m_address, ipAddress.m_port);

					serviceDiscoveryMessage.addEntry(serviceEntry);

					sendMessage(serviceDiscoveryMessage, channel);
				}
	}
}

void ServiceAnnouncer::onServiceUnregistered(const Service& service) {
	if (service.isLocal()) {
		TCPServer* server = m_tcpServer.getEndPoint(service);

		if (server != nullptr) {
			for (auto& channel : m_channels) {
				for (auto ipAddress : server->getIPAddresses())
					if (channel.address == ipAddress.getAddress()) {

						SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);

						SomeIPServiceDiscoveryServiceUnregisteredEntry serviceEntry(serviceDiscoveryMessage,
								service.getServiceIDs(), SomeIP::TransportProtocol::TCP, ipAddress.m_address,
								ipAddress.m_port);

						serviceDiscoveryMessage.addEntry(serviceEntry);

						sendMessage(serviceDiscoveryMessage, channel);
					}
			}
			server->removeService(service);
		}
	}
}

SomeIPReturnCode ServiceAnnouncer::init() {

	m_dispatcher.addServiceRegistrationListener(*this);

	auto interfaces = TCPServer::getAllIPAddresses();

	for (auto& address : interfaces) {

		struct in_addr localInterface;

		AnnouncementChannel channel;
		channel.address = address;

		channel.broadcastFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
		if (channel.broadcastFileDescriptor < 0)
			return SomeIPReturnCode::ERROR;

		auto& groupSock = channel.addr;

		memset((char *) &groupSock, 0, sizeof(groupSock));
		groupSock.sin_family = AF_INET;
		groupSock.sin_addr.s_addr = inet_addr(SERVICE_DISCOVERY_MULTICAST_ADDRESS);
		groupSock.sin_port = htons(SERVICE_DISCOVERY_UDP_PORT);

		/* Set local interface for outbound multicast datagrams. */
		/* The IP address specified must be associated with a local, */
		/* multicast capable interface. */
		localInterface.s_addr = inet_addr(channel.address.toString().c_str());
		if (setsockopt(channel.broadcastFileDescriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *) &localInterface,
				sizeof(localInterface)) < 0) {
			log_error() << "Setting local interface error";
			continue;
		}

		// Disable loopback so you do not receive your own datagrams.
		char loopch = 0;
		if (setsockopt(channel.broadcastFileDescriptor, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &loopch, sizeof(loopch))
				< 0) {
			log_error() << "Setting IP_MULTICAST_LOOP error";
			continue;
		}

		// send a query to know about services provided by other devices;
		SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);
		SomeIPServiceDiscoveryServiceQueryEntry entry(0xFFFF);
		serviceDiscoveryMessage.addEntry(entry);
		sendMessage(serviceDiscoveryMessage, channel);

		m_channels.push_back(channel);

		log_debug() << "Interface: " << channel.address.toString();

	}

	return SomeIPReturnCode::OK;
}

}
