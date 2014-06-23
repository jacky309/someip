#pragma once

#include <string>
#include <string.h>
#include <poll.h>
#include <cstdint>

#include "SomeIP-common.h"
#include "SomeIP.h"
#include "utilLib/GlibIO.h"

#include "ipc.h"

#include "Message.h"

#include "ipc/UDSConnection.h"

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
 * This class can be used to be notified of the availibilty/unavailability of a service
 */
class ServiceAvailabilityListener {
public:
	ServiceAvailabilityListener(SomeIP::ServiceID serviceID) {
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
	void onServiceRegistered(SomeIP::ServiceID serviceID) {
		if (serviceID == m_serviceID)
			onServiceAvailable();
	}

	void onServiceUnregistered(SomeIP::ServiceID serviceID) {
		if (serviceID == m_serviceID)
			onServiceUnavailable();
	}

private:
	SomeIP::ServiceID m_serviceID;

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
	const std::vector<SomeIP::ServiceID>& getAvailableServices() const {
		return m_availableServices;
	}

	/**
	 * Registers a new service availability listener
	 */
	void registerServiceAvailabilityListener(ServiceAvailabilityListener& listener) {
		m_serviceAvailabilityListeners.push_back(&listener);
	}

private:
	void onServiceRegistered(SomeIP::ServiceID serviceID) {
		m_availableServices.push_back(serviceID);
		for (auto& listener : m_serviceAvailabilityListeners) {
			listener->onServiceRegistered(serviceID);
		}
	}

	void onServiceUnregistered(SomeIP::ServiceID serviceID) {
		m_availableServices.push_back(serviceID);
		for (auto& listener : m_serviceAvailabilityListeners) {
			listener->onServiceUnregistered(serviceID);
		}
	}

private:
	std::vector<SomeIP::ServiceID> m_availableServices;
	std::vector<ServiceAvailabilityListener*> m_serviceAvailabilityListeners;

	friend class ClientDaemonConnection;
};

class OutputMessageWithReport : public OutputMessage {
public:
	OutputMessageWithReport(SomeIP::MessageID messageID) :
		OutputMessage(messageID) {
	}
	OutputMessageWithReport() :
		OutputMessage() {
	}
};

class ClientConnection {

public:

	virtual ~ClientConnection() {
	}

	struct MainLoopInterface {
		virtual ~MainLoopInterface() {
		}

		virtual void addIdleCallback(std::function<void()> callBackFunction) = 0;
	};

	/**
	 * Handles the data which has been received.
	 * @return : true if there is still some data to be processed
	 */
	virtual bool dispatchIncomingMessages() = 0;

	virtual void disconnect() = 0;

	virtual	void setMainLoopInterface(MainLoopInterface& mainloopInterface) = 0;

	virtual int getFileDescriptor() const = 0;

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
	virtual bool isConnected() const  = 0;

	/**
	 * Returns true if some data has been received
	 */
	virtual bool hasIncomingMessages() = 0;

	virtual ServiceRegistry& getServiceRegistry() = 0;

};


/**
 * This is the main class to be used to connect to the dispatcher
 */
class ClientDaemonConnection : public ClientConnection, private MessageSource, private UDSConnection {

	LOG_SET_CLASS_CONTEXT(clientLibContext);

public:

	static constexpr const char* DEFAULT_SERVER_SOCKET_PATH = "/tmp/someIPSocket";
	static constexpr const char* ALTERNATIVE_SERVER_SOCKET_PATH = "/tmp/someIPSocket2";

	ClientDaemonConnection() : m_endPoint(*this) {
	}

	~ClientDaemonConnection() {
		disconnect();
	}

	/**
	 * Returns true if some data has been received
	 */
	bool hasIncomingMessages() {
		return ( ( !m_queue.isEmpty() ) || hasAvailableBytes() );
	}

	int getFileDescriptor() const {
		return SocketStreamConnection::getFileDescriptor();
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
	 * Returns the service registry instance
	 */
	ServiceRegistry& getServiceRegistry() override {
		return m_registry;
	}

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
	SomeIPReturnCode registerService(SomeIP::ServiceID serviceID) override;

	/**
	 * Unregisters the given service
	 */
	SomeIPReturnCode unregisterService(SomeIP::ServiceID serviceID) override;

	/**
	 * Subscribes to notifications for the given MessageID
	 */
	SomeIPReturnCode subscribeToNotifications(SomeIP::MessageID messageID);

	/**
	 * Sends the given message to the dispatcher.
	 */
	SomeIPReturnCode sendMessage(OutputMessage& msg);

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
	std::string getDaemonStateDump();

	void setMainLoopInterface(MainLoopInterface& mainloopInterface) {
		m_mainLoop = &mainloopInterface;
	}

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
			const IPCInputMessage* msg = NULL;
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

	ClientConnectionListener* messageReceivedCallback = nullptr;

	std::vector<OutputMessageWithReport> m_messagesWithReport;

	MainLoopInterface* m_mainLoop = nullptr;

	ServiceRegistry m_registry;

	SafeMessageQueue m_queue;

	SomeIPEndPoint m_endPoint;

	std::recursive_mutex dataReceptionMutex;
	std::recursive_mutex dataEmissionMutex;

};


/**
 * This class integrates the dispatching of messages into a Glib main loop.
 */
class GLibIntegration : public GlibChannelListener, private ClientConnection::MainLoopInterface {

	LOG_SET_CLASS_CONTEXT(clientLibContext);

public:
	GLibIntegration(ClientConnection& connection, GMainContext* context = NULL) :
		m_connection(connection), m_channelWatcher(*this, context), m_idleCallBack(
			[&]() {
				m_idleCallbackFunction();
				return false;
			}) {
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
