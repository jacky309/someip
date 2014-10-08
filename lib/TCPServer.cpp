#include "TCPServer.h"
#include "TCPManager.h"
#include <ifaddrs.h>

namespace SomeIP_Dispatcher {

std::vector<IPV4Address> TCPServer::getAllIPAddresses() {

	std::vector<IPV4Address> ipAddresses;

	struct ifaddrs* ifaddr, * ifa;
	int family, s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1) {
		log_error() << "Error getting local IP addresses";
	} else {
		/* Walk through linked list, maintaining head pointer so we can free list later */
		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL)
				continue;

			family = ifa->ifa_addr->sa_family;

			if (family == AF_INET) {

				auto* p = reinterpret_cast<uint8_t*>(&( (struct sockaddr_in*) ifa->ifa_addr )->sin_addr);
				IPV4Address address(p);
				if ( !address.isLocalAddress() )
					ipAddresses.push_back(address);

				s =
					getnameinfo(ifa->ifa_addr,
						    (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
						    host, sizeof(host), nullptr, 0,
						    NI_NUMERICHOST);
				if (s != 0) {
					log_error() << "getnameinfo() failed: " << gai_strerror(s);
					throw new ConnectionExceptionWithErrno("getnameinfo() failed");
				}
			}
		}

		freeifaddrs(ifaddr);
	}

	return ipAddresses;
}

SomeIPReturnCode TCPServer::init(int portCount) {

	m_dispatcher.addBlackListFilter(*this);

	int tcpServerSocketHandle = 0;

	if ( ( tcpServerSocketHandle = ::socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) {
		log_error() << "Failed to create socket";
		return SomeIPReturnCode::ERROR;
	}

	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(m_port);
	sin.sin_addr.s_addr = INADDR_ANY;

	bool bindSuccessful = false;

	// we try to find an available TCP port to listen to
	for (int i = 0; i < portCount; i++) {
		sin.sin_port = htons(m_port);
		if (::bind( tcpServerSocketHandle, (struct sockaddr*) &sin, sizeof(sin) ) != 0) {
			log_warn() << "Failed to bind TCP server socket " << m_port << ". Error : " << strerror(errno);
			m_port++;
		} else {
			bindSuccessful = true;
			break;
		}
	}

	if (bindSuccessful) {
		if (::listen(tcpServerSocketHandle, SOMAXCONN) != 0) {
			log_error( "Failed to listen to TCP port %i. Error : %s", m_port, strerror(errno) );
			return SomeIPReturnCode::ERROR;
		}

		setFileDescriptor(tcpServerSocketHandle);

		log_info() << "TCP Server socket listening on port " << m_port;

		for ( auto address : getAllIPAddresses() ) {
			m_activePorts.push_back( IPv4TCPEndPoint(address, m_port) );
		}

		return SomeIPReturnCode::OK;
	}
	else {
		log_error() << "Failed to find a free port in the range " << m_port << "-" << m_port + portCount - 1;
		return SomeIPReturnCode::ERROR;
	}


}


bool TCPServer::isBlackListed(const IPv4TCPEndPoint& server, ServiceIDs serviceID) const {
	// ignore our own services
	for ( auto localAddress : m_activePorts )
		if ( localAddress == server ) {
			//			log_debug() << "Ignoring own service " << server.toString();
			return true;
		}

	return false;
}


void TCPServer::createNewClientConnection(int fileDescriptor) {
	TCPClient* newClient = new TCPClient(m_dispatcher, fileDescriptor, m_tcpManager, m_mainContext, m_tcpManager.getServiceNamespace());
	log_debug() << "New client : " << newClient->toString();
	newClient->registerClient();
}

}
