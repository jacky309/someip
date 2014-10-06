#pragma once

#include <string>
#include <string.h>
#include <poll.h>
#include <cstdint>

#include "SomeIP-common.h"
#include "SomeIP.h"

#include "ipc.h"

#include "Message.h"
#include "SomeIP-clientLib.h"

#include "Dispatcher.h"
#include "TCPManager.h"
#include "GlibMainLoopInterfaceImplementation.h"
#include "RemoteServiceListener.h"

namespace SomeIPClient {

using namespace SomeIP_Lib;
using namespace SomeIP_utils;
using namespace SomeIP_Dispatcher;

LOG_IMPORT_CONTEXT(clientLibContext);

/**
 * Interface to be used by client application to connect to the dispatcher. The dispatcher can either be local
 */
class DaemonLessClient : public ClientConnection, private Client {

	LOG_DECLARE_CLASS_CONTEXT("DLES" , "Daemonless connection");

public:
	DaemonLessClient(MainLoopContext& mainLoopContext) : Client(dispatcher), m_mainLoopContext(mainLoopContext), dispatcher(m_mainLoopContext)
, tcpManager(dispatcher, m_mainLoopContext)
, tcpServer(dispatcher, tcpManager, m_tcpPortNumber, m_mainLoopContext)
, serviceAnnouncer(dispatcher, tcpServer, m_mainLoopContext)
, remoteServiceListener(dispatcher, tcpManager, serviceAnnouncer, m_mainLoopContext)
{
	}

	virtual ~DaemonLessClient() {
	}

	/**
	 * Handles the data which has been received.
	 * @return : true if there is still some data to be processed
	 */
	bool dispatchIncomingMessages() override {
		assert(false);
	}

	void disconnect() override {
	}

	void init() override {
	}

	void onNotificationSubscribed(SomeIP::MemberID serviceID, SomeIP::MemberID memberID) override {
		assert(false);
	}

	/**
	 * Returns a string representation of the object
	 */
	std::string toString() const override {
		return std::string();

	}

	void sendMessage(const DispatcherMessage& msg) override {
//		dispatcher.dispatchMessage(msg, *this);
		assert(false);
	}

	/**
	 * Registers a new service.
	 */
	SomeIPReturnCode registerService(SomeIP::ServiceID serviceID) {
		auto service = dispatcher.tryRegisterService(serviceID, *this, true);

		if (service != nullptr)
			return SomeIPReturnCode::OK;
		else
			return SomeIPReturnCode::ERROR;

	}

	/**
	 * Unregisters the given service
	 */
	SomeIPReturnCode unregisterService(SomeIP::ServiceID serviceID) {
		assert(false);

	}

	/**
	 * Subscribes to notifications for the given MessageID
	 */
	 SomeIPReturnCode subscribeToNotifications(SomeIP::MessageID messageID) override {assert(false);
}

	/**
	 * Sends the given message to the dispatcher.
	 */
	 SomeIPReturnCode sendMessage(OutputMessage& msg) override {assert(false);
}

	/**
	 * Sends a ping message to the dispatcher
	 */
	 SomeIPReturnCode sendPing() override {assert(false);
}

	/**
	 * Sends the given message to the dispatcher and block until a response is received.
	 */
	 InputMessage sendMessageBlocking(const OutputMessage& msg) override {
		 assert(false);
		 log_debug() << msg;
	 }

	/**
	 * Connects to the dispatcher.
	 */
	 SomeIPReturnCode connect(ClientConnectionListener& clientReceiveCb) override {
		tcpServer.init(tcpPortTriesCount);

		for (auto& localIpAddress : tcpServer.getIPAddresses())
			log_debug() << "Local IP address : " << localIpAddress.toString();

		serviceAnnouncer.init();

		remoteServiceListener.init();

		return SomeIPReturnCode::OK;
	 }


	/**
	 * Returns true if the connection to the daemon is active
	 */
	bool isConnected() const override {
		return tcpServer.isConnected();
	}

	/**
	 * Returns true if some data has been received
	 */
	 bool hasIncomingMessages() override {
		assert(false);
		return false;
	 }

	bool isServiceAvailableBlocking(ServiceID service) override {
		assert(false);
		return false;
	}

private:
	int m_tcpPortNumber = 6666;
	int tcpPortTriesCount = 1;

	MainLoopContext& m_mainLoopContext;
	Dispatcher dispatcher;
	TCPManager tcpManager;
	TCPServer tcpServer;
	ServiceAnnouncer serviceAnnouncer;
	RemoteServiceListener remoteServiceListener;

};

}
