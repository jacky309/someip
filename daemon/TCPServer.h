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

typedef uint16_t TCPPort;

class TCPServer : private SocketStreamServer, private BlackListHostFilter {

	LOG_DECLARE_CLASS_CONTEXT("TCPS", "TCPServer");

public:
	static const TCPPort DEFAULT_TCP_SERVER_PORT = 10032;

	TCPServer(Dispatcher& dispatcher, TCPManager& tcpManager, TCPPort port) :
		m_dispatcher(dispatcher), m_tcpManager(tcpManager), m_port(port) {
	}

	virtual ~TCPServer() {
	}

	bool isBlackListed(const IPv4TCPEndPoint& server, ServiceID serviceID) const override;

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

	const std::vector<IPv4TCPEndPoint> getIPAddresses() const {
		return m_activePorts;
	}

	static std::vector<IPV4Address> getAllIPAddresses();

	void init(int portCount = 1);

private:
	std::vector<IPv4TCPEndPoint> m_activePorts;
	Dispatcher& m_dispatcher;
	TCPManager& m_tcpManager;
	TCPPort m_port = -1;
};

}
