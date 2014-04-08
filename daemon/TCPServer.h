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

	static std::vector<IPV4Address> getIPAddresses();

	void initServerSocket();

	void init() {
		initServerSocket();
	}

private:
	Dispatcher& m_dispatcher;
	TCPManager& m_tcpManager;
	int m_port = -1;
};

}
