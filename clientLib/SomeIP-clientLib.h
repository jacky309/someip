#pragma once

#include <string>
#include <string.h>
#include <poll.h>
#include <cstdint>

#include "SomeIP-common.h"
#include "SomeIP.h"

#include "ipc.h"

#include "Message.h"

#include "ipc/UDSConnection.h"
#include <algorithm>

namespace SomeIPClient {

using namespace SomeIP_Lib;
using namespace SomeIP_utils;

LOG_IMPORT_CONTEXT(clientLibContext);

/**
 * This interface needs to be implemented to handle incoming messages
 */
class ClientConnectionListener : public MessageSink {
public:
	/**
	 * Called when the connection to the dispatcher has been lost
	 */
	virtual void onDisconnected() = 0;
};



/**
 * This class can be used to be notified of the availability/unavailability of a service
 */
class ServiceAvailabilityListener {
public:
	ServiceAvailabilityListener(SomeIP::ServiceIDs serviceID) {
		m_serviceID = serviceID;
	}

	virtual ~ServiceAvailabilityListener() {
	}

	/**
	 * This method is called when the service has become available
	 */
	virtual void onServiceAvailable() = 0;

	/**
	 * This method is called when the service has become unavailable
	 */
	virtual void onServiceUnavailable() = 0;

private:
	void onServiceRegistered(SomeIP::ServiceIDs serviceID) {
		if (serviceID.matches(m_serviceID))
			onServiceAvailable();
	}

	void onServiceUnregistered(SomeIP::ServiceIDs serviceID) {
		if (serviceID.matches(m_serviceID))
			onServiceUnavailable();
	}

private:
	SomeIP::ServiceIDs m_serviceID;

	friend class ServiceRegistry;

};

/**
 * This class maintains a list of available services and lets you register a listener to be notified whenever a new service
 * has been registered or unregistered.
 */
class ServiceRegistry {

public:
	/**
	 * Returns the list of currently registered services
	 */
	const std::vector<SomeIP::ServiceIDs>& getAvailableServices() const {
		return m_availableServices;
	}

	bool isServiceRegistered(SomeIP::ServiceIDs serviceID) const {
		bool bfound = (std::find(m_availableServices.begin(), m_availableServices.end(), serviceID) != m_availableServices.end());
		return bfound;
	}

	/**
	 * Registers a new service availability listener
	 */
	void registerServiceAvailabilityListener(ServiceAvailabilityListener& listener) {
		m_serviceAvailabilityListeners.push_back(&listener);
	}

protected:
	void onServiceRegistered(SomeIP::ServiceIDs serviceID) {
		m_availableServices.push_back(serviceID);
		for (auto& listener : m_serviceAvailabilityListeners) {
			listener->onServiceRegistered(serviceID);
		}
	}

	void onServiceUnregistered(SomeIP::ServiceIDs serviceID) {
		m_availableServices.erase(std::find(m_availableServices.begin(), m_availableServices.end(), serviceID));
		for (auto& listener : m_serviceAvailabilityListeners) {
			listener->onServiceUnregistered(serviceID);
		}
	}

private:
	std::vector<SomeIP::ServiceIDs> m_availableServices;
	std::vector<ServiceAvailabilityListener*> m_serviceAvailabilityListeners;

	friend class ClientConnection;
};

class OutputMessageWithReport : public OutputMessage {
public:
	OutputMessageWithReport(SomeIP::MemberIDs messageID) :
		OutputMessage(messageID) {
	}
	OutputMessageWithReport() :
		OutputMessage() {
	}
};

/**
 * Interface to be used by client application to connect to the dispatcher. The dispatcher can either be local
 */
class ClientConnection {

public:

	ClientConnection() {
	}

	virtual ~ClientConnection() {
	}

	/**
	 * Handles the data which has been received.
	 * @return : true if there is still some data to be processed
	 */
	virtual bool dispatchIncomingMessages() = 0;

	virtual void disconnect() = 0;

	/**
	 * Registers a new service.
	 */
	virtual SomeIPReturnCode registerService(SomeIP::ServiceIDs serviceID) = 0;

	/**
	 * Unregisters the given service
	 */
	virtual SomeIPReturnCode unregisterService(SomeIP::ServiceIDs serviceID) = 0;

	/**
	 * Subscribes to notifications for the given MessageID
	 */
	virtual SomeIPReturnCode subscribeToNotifications(SomeIP::MemberIDs memberID) = 0;

	/**
	 * Sends the given message to the dispatcher.
	 */
	virtual SomeIPReturnCode sendMessage(const OutputMessage& msg) = 0;

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

	void setMainLoopInterface(MainLoopInterface& mainloopInterface) {
		m_mainLoop = &mainloopInterface;
	}

	/**
	 * Returns the service registry instance
	 */
	ServiceRegistry& getServiceRegistry() {
		return m_registry;
	}

	virtual bool isServiceAvailableBlocking(ServiceIDs service) = 0;

protected:

	void onServiceRegistered(SomeIP::ServiceIDs serviceID) {
		m_registry.onServiceRegistered(serviceID);
	}

	void onServiceUnregistered(SomeIP::ServiceIDs serviceID) {
		m_registry.onServiceUnregistered(serviceID);
	}

	MainLoopInterface* m_mainLoop = nullptr;
	ClientConnectionListener* messageReceivedCallback = nullptr;
	ServiceRegistry m_registry;

};


/**
 * This is the main class to be used to connect to the daemon dispatcher
 */
class ClientDaemonConnection : public ClientConnection, private MessageSource, private UDSConnection {

	LOG_SET_CLASS_CONTEXT(clientLibContext);

public:
	static constexpr const char* DEFAULT_SERVER_SOCKET_PATH = "/tmp/someIPSocket";
	static constexpr const char* ALTERNATIVE_SERVER_SOCKET_PATH = "/tmp/someIPSocket2";

	ClientDaemonConnection() : m_endPoint(*this) {
	}

	~ClientDaemonConnection() {
		messageReceivedCallback = nullptr;
		disconnect();
	}

	/**
	 * Returns true if some data has been received
	 */
	bool hasIncomingMessages() {
		return ( ( !m_queue.isEmpty() ) || hasAvailableBytes() );
	}

	void disconnect() {
		SocketStreamConnection::disconnect();
	}

	/**
	 * Handles the data which has been received.
	 * @return : true if there is still some data to be processed
	 */
	bool dispatchIncomingMessages() override;

	/**
	 * Connects to the dispatcher.
	 */
	SomeIPReturnCode connect(ClientConnectionListener& clientReceiveCb) override;

	/**
	 * Returns true if the connection to the daemon is active
	 */
	bool isConnected() const {
		return UDSConnection::isConnected();
	}

	/**
	 * Registers a new service.
	 */
	SomeIPReturnCode registerService(SomeIP::ServiceIDs serviceID) override;

	/**
	 * Unregisters the given service
	 */
	SomeIPReturnCode unregisterService(SomeIP::ServiceIDs serviceID) override;

	/**
	 * Subscribes to notifications for the given MessageID
	 */
	SomeIPReturnCode subscribeToNotifications(SomeIP::MemberIDs memberID) override;

	/**
	 * Sends the given message to the dispatcher.
	 */
	SomeIPReturnCode sendMessage(const OutputMessage& msg);

	/**
	 * Sends a ping message to the dispatcher
	 */
	SomeIPReturnCode sendPing();

	/**
	 * Sends the given message to the dispatcher and block until a response is received.
	 */
	InputMessage sendMessageBlocking(const OutputMessage& msg);

	/**
	 * Returns a dump of the daemon's internal state, which can be useful for diagnostic.
	 */
	SomeIPReturnCode getDaemonStateDump(std::string& dump);

	bool isServiceAvailableBlocking(ServiceIDs service) override;

private:
	class SafeMessageQueue {

public:
		SafeMessageQueue() {
			queueHeadIndex = 0;
		}

		void push(const IPCInputMessage& msg) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_messageQueue.push_back( new IPCInputMessage(msg) );
		}

		const IPCInputMessage* pop() {
			const IPCInputMessage* msg = nullptr;
			std::lock_guard<std::mutex> lock(m_mutex);
			if ( !isEmpty() )
				msg = m_messageQueue[queueHeadIndex++];
			else {
				queueHeadIndex = 0;
				m_messageQueue.resize(0);
			}

			return msg;
		}

		bool isEmpty() {
			return ( queueHeadIndex == m_messageQueue.size() );
		}

private:
		std::vector<IPCInputMessage*> m_messageQueue;
		size_t queueHeadIndex;
		std::mutex m_mutex;

	};

	std::string toString() const {
		return "ClientConnection";
	}

	InputMessage waitForAnswer(const OutputMessage& requestMsg);

	void pushToQueue(const IPCInputMessage& msg);

	IPCInputMessage writeRequest(IPCOutputMessage& ipcMessage);

	SomeIPReturnCode writeMessage(const IPCMessage& ipcMessage) {
		std::lock_guard<std::recursive_mutex> lock(dataEmissionMutex);
		auto code = writeBlocking(ipcMessage);
		return ( (code == IPCOperationReport::OK) ? SomeIPReturnCode::OK : SomeIPReturnCode::ERROR );
	}

	bool enqueueIncomingMessages() {
		return readIncomingMessages([&] (IPCInputMessage & msg) {
						    pushToQueue(msg);
						    return true;
					    });
	}

	void dispatchQueuedMessages();

	void onIPCInputMessage(IPCInputMessage& inputMessage) {
		dispatchQueuedMessages();
		handleIncomingIPCMessage(inputMessage);
	}

	void handleIncomingIPCMessage(IPCInputMessage& inputMessage) override {
		handleConstIncomingIPCMessage(inputMessage);
	}

	void handleConstIncomingIPCMessage(const IPCInputMessage& inputMessage);

	void onCongestionDetected() override;

	void onDisconnected() override;

	void newInputMessage() {
		IPCInputMessage* msg = new IPCInputMessage();
		setInputMessage(*msg);
	}

	IPCOperationReport readIncomingMessagesBlocking(IPCMessageReceivedCallbackFunction dispatchFunction);

	bool readIncomingMessages(IPCMessageReceivedCallbackFunction dispatchFunction);

	void onConnectionLost() {
		log_error() << "Connection to daemon lost";
	}

	std::vector<OutputMessageWithReport> m_messagesWithReport;

	SafeMessageQueue m_queue;

	SomeIPEndPoint m_endPoint;

	std::recursive_mutex dataReceptionMutex;
	std::recursive_mutex dataEmissionMutex;

	std::unique_ptr<WatchMainLoopHook> m_bufferedMessagesWatch;
	std::unique_ptr<WatchMainLoopHook> m_inputDataWatch;
	std::unique_ptr<WatchMainLoopHook> m_disconnectionWatch;

	int m_queuedMessageIndicatorPipe[2];
	char m_dummy = 0;

};

}
