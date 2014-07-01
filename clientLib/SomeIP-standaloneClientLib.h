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


namespace SomeIPClient {

using namespace SomeIP_Lib;
using namespace SomeIP_utils;

LOG_IMPORT_CONTEXT(clientLibContext);

/**
 * Interface to be used by client application to connect to the dispatcher. The dispatcher can either be local
 */
class DaemonLessClient : public ClientConnection {

public:
	virtual ~DaemonLessClient() {
	}

	/**
	 * Handles the data which has been received.
	 * @return : true if there is still some data to be processed
	 */
	bool dispatchIncomingMessages() override {
		return false;
	}

	void disconnect() override {
	}

	/**
	 * Registers a new service.
	 */
	virtual SomeIPReturnCode registerService(SomeIP::ServiceID serviceID) = 0;

	/**
	 * Unregisters the given service
	 */
	virtual SomeIPReturnCode unregisterService(SomeIP::ServiceID serviceID) = 0;

	/**
	 * Subscribes to notifications for the given MessageID
	 */
	virtual SomeIPReturnCode subscribeToNotifications(SomeIP::MessageID messageID) = 0;

	/**
	 * Sends the given message to the dispatcher.
	 */
	virtual SomeIPReturnCode sendMessage(OutputMessage& msg) = 0;

	/**
	 * Sends a ping message to the dispatcher
	 */
	virtual SomeIPReturnCode sendPing() = 0;

	/**
	 * Sends the given message to the dispatcher and block until a response is received.
	 */
	virtual InputMessage sendMessageBlocking(const OutputMessage& msg) = 0;

	/**
	 * Connects to the dispatcher.
	 */
	virtual SomeIPReturnCode connect(ClientConnectionListener& clientReceiveCb) = 0;

	/**
	 * Returns true if the connection to the daemon is active
	 */
	virtual bool isConnected() const = 0;

	/**
	 * Returns true if some data has been received
	 */
	virtual bool hasIncomingMessages() = 0;

	virtual ServiceRegistry& getServiceRegistry() = 0;

};

}
