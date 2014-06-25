#pragma once

#include "SomeIP-clientLib.h"
#include "utilLib/GlibIO.h"

namespace SomeIPClient {

/**
 * This class integrates the dispatching of messages into a Glib main loop.
 */
class GLibIntegration : public GlibChannelListener, private ClientConnection::MainLoopInterface {

	LOG_SET_CLASS_CONTEXT(clientLibContext);

public:
	GLibIntegration(ClientConnection& connection) :
		m_connection(connection), m_channelWatcher(*this, nullptr), m_idleCallBack(
			[&]() {
				m_idleCallbackFunction();
				return false;
			}, nullptr) {
		m_connection.setMainLoopInterface(*this);
	}

	~GLibIntegration() {
		onDisconnected();
	}

	void setup() {
		m_channelWatcher.setup( m_connection.getFileDescriptor() );
		m_channelWatcher.enableWatch();
	}

private:
	void addIdleCallback(std::function<void()> callBackFunction) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_idleCallbackFunction = callBackFunction;
		m_idleCallBack.activate();
	}

	void onDisconnected() override {
		m_connection.disconnect();
	}

	WatchStatus onWritingPossible() override {
		return WatchStatus::STOP_WATCHING;
	}

	WatchStatus onIncomingDataAvailable() override {
		m_connection.dispatchIncomingMessages();
		return WatchStatus::KEEP_WATCHING;
	}

	std::function<void()> m_idleCallbackFunction;

	ClientConnection& m_connection;

	std::mutex m_mutex;

	GlibChannelWatcher m_channelWatcher;

	GlibIdleCallback m_idleCallBack;
};

}
