#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include "RemoteServiceListener.h"
#include "ServiceDiscovery.h"
#include "TCPManager.h"

namespace SomeIP_Dispatcher {

void RemoteServiceListener::handleMessage() {
	ByteArray array;
	array.resize(65536);

	int bytes = read( m_broadcastFileDescriptor, array.getData(), array.size() );
	assert( bytes <= static_cast<int>( array.size() ) );

	array.resize(bytes);
	m_serviceDiscoveryDecoder.decodeMessage( array.getData(), array.size() );
}

SomeIPReturnCode RemoteServiceListener::init() {

	m_broadcastFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_broadcastFileDescriptor < 0)
		return SomeIPReturnCode::ERROR;

	int so_reuseaddr = TRUE;
	if (setsockopt( m_broadcastFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (so_reuseaddr) ) != 0) {
		log_error () << "Can't set SO_REUSEADDR";
		return SomeIPReturnCode::ERROR;
	}

	struct sockaddr_in addr;
	memset( &addr, 0, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(SERVICE_DISCOVERY_UDP_PORT);

	if (::bind( m_broadcastFileDescriptor, (struct sockaddr*) &addr, sizeof(addr) ) == 0) {
		struct ip_mreq group;
		group.imr_multiaddr.s_addr = inet_addr(SERVICE_DISCOVERY_BROADCAST_ADDRESS);
		group.imr_interface.s_addr = INADDR_ANY; //inet_addr(f);
		if (setsockopt(m_broadcastFileDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &group, sizeof(group)) < 0) {
			log_error() << "Adding multicast group error";
			return SomeIPReturnCode::ERROR;
		}

		pollfd fd;
		fd.fd = m_broadcastFileDescriptor;
		fd.events = POLLIN;
		m_inputDataWatcher = m_mainLoopContext.addWatch([&] () {
									handleMessage();
								}, fd);
		m_inputDataWatcher->enable();

	} else {
		log_error() << "Can't bind socket";
		return SomeIPReturnCode::ERROR;
	}

	return SomeIPReturnCode::OK;
}

}
