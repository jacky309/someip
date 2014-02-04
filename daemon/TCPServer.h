#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include "glib.h"

#include "SomeIP-common.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "LocalClient.h"
#include "SocketStreamConnection.h"
#include "TCPClient.h"

namespace SomeIP_Dispatcher {

class TCPServer : private SocketStreamServer {

	LOG_DECLARE_CLASS_CONTEXT("TCPS", "TCPServer");

public:
	TCPServer(Dispatcher& dispatcher, TCPManager& tcpManager) :
		m_dispatcher(dispatcher), m_tcpManager(tcpManager) {
	}

	virtual ~TCPServer() {
	}

	static gboolean onNewSocketConnection(GIOChannel* gio, GIOCondition condition, gpointer data) {
		TCPServer* server = static_cast<TCPServer*>(data);
		return server->handleNewConnection();
	}

	void createNewClientConnection(int fileDescriptor) override {
		TCPClient* newClient = new TCPClient(m_dispatcher, fileDescriptor, m_tcpManager);
		log_debug( "New client : %s", newClient->toString().c_str() );
		newClient->registerClient();
	}

	void onClientDisconnected(const Client& client) {
		delete &client;
	}

	int getLocalTCPPort() {
		return m_port;
	}

	static std::vector<IPV4Address> getIPAddresses() {

		std::vector<IPV4Address> ipAddresses;

		struct ifaddrs* ifaddr, * ifa;
		int family, s;
		char host[NI_MAXHOST];

		if (getifaddrs(&ifaddr) == -1) {
			log_error("Error getting local IP addresses");
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
							    (family ==
							     AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
							    host, sizeof(host), NULL, 0, NI_NUMERICHOST);
					if (s != 0) {
						log_error( "getnameinfo() failed: %s\n", gai_strerror(s) );
						exit(EXIT_FAILURE);
					}
				}
			}

			freeifaddrs(ifaddr);
		}

		return ipAddresses;
	}

	void initServerSocket() {

		int tcpServerSocketHandle = 0;
		m_port = getConfiguration().getDefaultLocalTCPPort();

		if ( ( tcpServerSocketHandle = ::socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) {
			log_error("Failed to create socket");
			throw ConnectionExceptionWithErrno("Failed to create socket");
		}

		struct sockaddr_in sin;

		sin.sin_family = AF_INET;
		sin.sin_port = htons(m_port);
		sin.sin_addr.s_addr = INADDR_ANY;

		if (true) {
			// we try to find an available TCP port to listen to
			for (int i = 0; i < 10; i++) {
				sin.sin_port = htons(m_port);
				if (::bind( tcpServerSocketHandle, (struct sockaddr*) &sin, sizeof(sin) ) != 0) {
					log_error( "Failed to bind TCP server socket %d. Error : %s", m_port + i, strerror(errno) );
					m_port++;
				} else
					break;
			}
		} else if (::bind( tcpServerSocketHandle, (struct sockaddr*) &sin, sizeof(sin) ) != 0) {
			log_error( "Failed to bind TCP server socket %d. Error : %s", m_port, strerror(errno) );
			throw ConnectionExceptionWithErrno("Failed to bind TCP server socket");
		}

		if (::listen(tcpServerSocketHandle, SOMAXCONN) != 0) {
			log_error( "Failed to listen to TCP port %i. Error : %s", m_port, strerror(errno) );
			throw ConnectionExceptionWithErrno("Failed to listen to TCP port");
		}

		GIOChannel* tcpServerSocketChannel = g_io_channel_unix_new(tcpServerSocketHandle);

		if ( !g_io_add_watch(tcpServerSocketChannel, (GIOCondition)(G_IO_IN | G_IO_HUP), onNewSocketConnection, this) ) {
			log_error("Cannot add watch on TCP GIOChannel!");
			throw ConnectionException("Failed to listen to TCP port");
		}

		log_info("TCP Server socket listening on port %d", m_port);

		setFileDescriptor(tcpServerSocketHandle);
	}

	void init() {
		initServerSocket();
	}

	Dispatcher& m_dispatcher;

	TCPManager& m_tcpManager;

	int m_port = -1;
};

}
