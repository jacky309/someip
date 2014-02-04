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
#include "ServiceDiscovery.h"
#include "TCPClient.h"

namespace SomeIP_Dispatcher {

class RemoteServiceListener {

	LOG_DECLARE_CLASS_CONTEXT("RSL", "Remote service listener");

public:
	RemoteServiceListener(Dispatcher& dispatcher, TCPManager& tcpManager) :
		m_tcpManager(tcpManager) {
	}

	~RemoteServiceListener() {
		g_io_channel_unref(m_serverSocketChannel);
	}

	static gboolean onMessageGlibCallback(GIOChannel* gio, GIOCondition condition, gpointer data) {
		RemoteServiceListener* server = static_cast<RemoteServiceListener*>(data);
		server->handleMessage();
		return true;
	}

	void init() {

		struct sockaddr_in addr;

		m_broadcastFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_broadcastFileDescriptor < 0)
			throw ConnectionExceptionWithErrno("Can't create socket");

		memset( &addr, 0, sizeof(addr) );
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(SERVICE_DISCOVERY_UDP_PORT);

		if (::bind( m_broadcastFileDescriptor, (struct sockaddr*) &addr, sizeof(addr) ) != 0)
			throw ConnectionExceptionWithErrno("Can't bind socket");

		m_serverSocketChannel = g_io_channel_unix_new(m_broadcastFileDescriptor);

		if ( !g_io_add_watch(m_serverSocketChannel, (GIOCondition)(G_IO_IN | G_IO_HUP), onMessageGlibCallback, this) ) {
			log_error("Cannot add watch on GIOChannel");
			throw ConnectionExceptionWithErrno("Cannot add watch on GIOChannel");
		}

	}

	void handleMessage();

	int m_broadcastFileDescriptor = -1;

	GIOChannel* m_serverSocketChannel = NULL;
	TCPManager& m_tcpManager;

};

}
