#pragma once

#include <exception>
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
		return queueHeadIndex == m_messageQueue.size();
	}

private:
	std::vector<IPCInputMessage*> m_messageQueue;
	size_t queueHeadIndex;
	std::mutex m_mutex;

};


class ClientConnectionListener : public MessageSink {
public:
	virtual void onDisconnected() = 0;
};


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
 *
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
	 * Register a new service availability listener
	 */
	void registerServiceAvailabilityListener(ServiceAvailabilityListener& listener) {
		m_serviceAvailabilityListeners.push_back(&listener);
	}

private:
	ServiceRegistry() {
	}

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

	friend class ClientConnection;
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

class RegistrationException : public std::exception {
public:
	RegistrationException() {
	}

	RegistrationException(const char* err, SomeIP::ServiceID& service) {
		char message[strlen(err) + 20];
		snprintf(message, sizeof(message), "%s, 0x%X", err, service);
		m_err = message;
	}

	virtual ~RegistrationException() throw (){
	}

	const char* what() const throw (){
		return m_err.c_str();
	}

private:
	std::string m_err;
};

/**
 * This is the main class to be used to connect to the dispatcher
 */
class ClientConnection : private MessageSource, private UDSConnection {

	LOG_SET_CLASS_CONTEXT(clientLibContext);

public:
	using SocketStreamConnection::getFileDescriptor;
	using SocketStreamConnection::disconnect;

	struct MainLoopInterface {
		virtual ~MainLoopInterface() {
		}

		virtual void addIdleCallback(std::function<void()> callBackFunction) = 0;
	};

	ClientConnection() : m_endPoint(*this) {
	}

	~ClientConnection() {
		disconnect();
	}

	bool hasIncomingMessages() {
		return ( ( !m_queue.isEmpty() ) || hasAvailableBytes() );
	}

	bool dispatchIncomingMessages();

	/**
	 * Returns the service registry instance
	 */
	ServiceRegistry& getServiceRegistry() {
		return m_registry;
	}

	/**
	 * Connect to the dispatcher.
	 * @exception ConnectionException is thrown if the connection can not be established
	 */
	void connect(ClientConnectionListener& clientReceiveCb);

	/**
	 * Return true if the connection to the daemon is active
	 */
	bool isConnected() const {
		return UDSConnection::isConnected();
	}

	/**
	 * Register a new service.
	 */
	void registerService(SomeIP::ServiceID serviceID);

	/**
	 * Unregister the given service
	 */
	void unregisterService(SomeIP::ServiceID serviceID);

	/**
	 * Subscribe to notifications for the given MessageID
	 */
	void subscribeToNotifications(SomeIP::MessageID messageID);


	/**
	 * Send the given message to the dispatcher.
	 */
	SomeIPFunctionReturnCode sendMessage(OutputMessage& msg);

	/**
	 * Send the given message to the dispatcher and block until a response is received.
	 */
	InputMessage sendMessageBlocking(const OutputMessage& msg);

	/**
	 * Returns a dump of the daemon's internal state, which can be useful for diagnostic.
	 */
	std::string getDaemonStateDump();

	void sendPing() {
		IPCOutputMessage ipcMessage(IPCMessageType::PONG);
		writeMessage(ipcMessage);
	}

	void setMainLoopInterface(MainLoopInterface& mainloopInterface) {
		m_mainLoop = &mainloopInterface;
	}

private:
	std::string toString() const {
		return "ClientConnection";
	}

	InputMessage waitForAnswer(const OutputMessage& requestMsg);

	void pushToQueue(const IPCInputMessage& msg);

	IPCInputMessage writeRequest(IPCOutputMessage& ipcMessage);

	void writeMessage(const IPCMessage& ipcMessage) {
		std::lock_guard<std::recursive_mutex> lock(dataEmissionMutex);
		writeBlocking(ipcMessage);
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


	void swapInputMessage() {
		//		log_info("swap");
		IPCInputMessage* msg = new IPCInputMessage();
		setInputMessage(*msg);
	}

	IPCOperationReport readIncomingMessagesBlocking(IPCMessageReceivedCallbackFunction dispatchFunction);


	bool readIncomingMessages(IPCMessageReceivedCallbackFunction dispatchFunction);

	void onConnectionLost() {
		log_error("Connection to daemon lost");
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
