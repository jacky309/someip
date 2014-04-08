#pragma once


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

	void init();

private:
	void handleMessage();

	static gboolean onMessageGlibCallback(GIOChannel* gio, GIOCondition condition, gpointer data) {
		RemoteServiceListener* server = static_cast<RemoteServiceListener*>(data);
		server->handleMessage();
		return true;
	}

	int m_broadcastFileDescriptor = -1;

	GIOChannel* m_serverSocketChannel = NULL;
	TCPManager& m_tcpManager;

};

}
