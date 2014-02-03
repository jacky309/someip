#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include "glib.h"

#include "log.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "LocalClient.h"
#include "ipc/UDSConnection.h"

#ifdef ENABLE_SYSTEMD
#include "sd-daemon.h"
#endif

/**
 * Handles the local connections.
 */
class LocalServer : private UDSServer {

	LOG_DECLARE_CLASS_CONTEXT("LoSe", "LocalServer");

public:
	LocalServer(Dispatcher& dispatcher) :
		m_dispatcher(dispatcher) {
		setSocketPath(DEFAULT_SERVER_SOCKET_PATH);
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
		LocalClient* newClient = new LocalClient(m_dispatcher, fileDescriptor);
		newClient->registerClient();
		log_debug( "New client : %s", newClient->toString().c_str() );
	}

	void onClientDisconnected(const Client& client) {
		delete &client;
	}

	void init() {

#ifdef ENABLE_SYSTEMD
		int systemdPassedSockets = sd_listen_fds(false);

		if (systemdPassedSockets == 1) {
			// we got a file descriptor from systemd => use it instead of creating a new one
			m_fileDescriptor = SD_LISTEN_FDS_START;
			log_info("Got a file descriptor from systemd");
		} else
			initServerSocket();
#else
		initServerSocket();
#endif

		m_serverSocketChannel = g_io_channel_unix_new( getFileDescriptor() );

		if ( !g_io_add_watch(m_serverSocketChannel, (GIOCondition)(G_IO_IN | G_IO_HUP), onNewSocketConnection, this) ) {
			log_error("Cannot add watch on GIOChannel");
			throw ConnectionException("Cannot add watch on GIOChannel");
		}

		// when running as root, allow the applications which are not running as root to connect
		if ( chmod(getSocketPath(), S_IRWXU | S_IRWXG // | S_IRWXO
			   ) ) {
			log_warn("Cannot set the permission on the socket");
			//			throw ConnectionException("Cannot set the permission on the socket");
		}

		log_info("UNIX domain server socket listening on path") << getSocketPath();

	}

	Dispatcher& m_dispatcher;
	GIOChannel* m_serverSocketChannel = nullptr;
};
