#pragma once

#include "glib.h"

#include "SomeIP-common.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "LocalClient.h"
#include "ipc/UDSConnection.h"

#ifdef ENABLE_SYSTEMD
#include "sd-daemon.h"
#endif

namespace SomeIP_Dispatcher {

/**
 * Handles the local connections.
 */
class LocalServer : private UDSServer {

	LOG_DECLARE_CLASS_CONTEXT("LoSe", "LocalServer");

public:
	LocalServer(Dispatcher& dispatcher, MainLoopContext& mainLoopContext) : UDSServer(mainLoopContext),
		m_dispatcher(dispatcher), m_mainLoopContext(mainLoopContext) {
	}

	~LocalServer() {
		g_io_channel_unref(m_serverSocketChannel);

		// delete server socket. TODO : check whether the socket should actually be removed with SystemD
		if (m_serverSocketChannel != nullptr)
			remove( getSocketPath() );
	}

	static gboolean onNewSocketConnection(GIOChannel* gio, GIOCondition condition, gpointer data) {
		LocalServer* server = static_cast<LocalServer*>(data);
		return server->handleNewConnection();
	}

	void createNewClientConnection(int fileDescriptor) override {
		LocalClient* newClient = new LocalClient(m_dispatcher, fileDescriptor, m_mainLoopContext);
		newClient->registerClient();
		log_debug() << "New client : " << newClient->toString();
	}

	void onClientDisconnected(const Client& client) {
		delete &client;
	}

	void init(const char* socketPath);

private:
	Dispatcher& m_dispatcher;
	GIOChannel* m_serverSocketChannel = nullptr;
	MainLoopContext& m_mainLoopContext;
};

}
