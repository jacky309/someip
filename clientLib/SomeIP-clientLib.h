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

//typedef MessageProcessor MessageReceivedCallbackFunction;

class ClientConnection;

//ClientConnection& getClientConnectionSingleton();

class ClientConnection : private MessageSource, private UDSConnection {

	LOG_SET_CLASS_CONTEXT(clientLibContext);

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

public:
	using SocketStreamConnection::getFileDescriptor;
	using SocketStreamConnection::disconnect;

	static bool isError(SomeIPFunctionReturnCode code) {
		return (code != SomeIPFunctionReturnCode::OK);
	}

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

	//	static ClientConnection& getSingleton() {
	//		return getClientConnectionSingleton();
	//	}

	bool hasIncomingMessages() {
		return ( ( !m_queue.isEmpty() ) || hasAvailableBytes() );
	}

	bool dispatchIncomingMessages() {
		dispatchQueuedMessages();
		return readIncomingMessages([&] (IPCInputMessage & msg) {
						    onIPCInputMessage(msg);
						    return true;
					    });
	}

	/**
	 * Returns the service registry instance, which
	 */
	ServiceRegistry& getServiceRegistry() {
		return m_registry;
	}

	/**
	 * Connect to the dispatcher.
	 * @exception ConnectionException is thrown if the connection can not be established
	 */
	void connect(ClientConnectionListener& clientReceiveCb) {

		if ( isConnected() )
			throw ConnectionException("Already connected !");

		/* register receive callback */
		messageReceivedCallback = &clientReceiveCb;

		swapInputMessage();

		connectToServer();

		log_info( "Big endian : %i", isNativeBigEndian() );

	}

	bool isConnected() const {
		return UDSConnection::isConnected();
	}

	/**
	 * Register a new service.
	 */
	void registerService(SomeIP::ServiceID serviceID) {
		IPCOutputMessage msg(IPCMessageType::REGISTER_SERVICE);
		msg << serviceID;
		IPCInputMessage returnMessage = writeRequest(msg);

		if (returnMessage.getReturnCode() == IPCReturnCode::ERROR)
			throw RegistrationException("Can't register service", serviceID);
		else
			log_info("Successfully registered service 0x%X", serviceID);
	}

	/**
	 * Unregister the given service
	 */
	void unregisterService(SomeIP::ServiceID serviceID) {
		IPCOutputMessage msg(IPCMessageType::UNREGISTER_SERVICE);
		msg << serviceID;
		IPCInputMessage returnMessage = writeRequest(msg);

		if (returnMessage.getReturnCode() == IPCReturnCode::ERROR)
			throw RegistrationException("Can't unregister service", serviceID);
		else
			log_info("Successfully unregistered service 0x%X", serviceID);
	}

	/**
	 * Subscribe to notifications for the given MessageID
	 */
	void subscribeToNotifications(SomeIP::MessageID messageID) {
		log_debug("Subscribing to notifications 0x%X", messageID);
		IPCOutputMessage msg(IPCMessageType::SUBSCRIBE_NOTIFICATION);
		msg << messageID;
		writeMessage(msg);
	}

	/**
	 * Send the given message to the dispatcher.
	 */
	SomeIPFunctionReturnCode sendMessage(OutputMessage& msg) {
		const IPCMessage& ipcMessage = msg.getIPCMessage();
		writeMessage(ipcMessage);
#ifdef ENABLE_TRAFFIC_LOGGING
		log_debug( "Message sent : %s", msg.toString().c_str() );
#endif
		return SomeIPFunctionReturnCode::OK;
	}

	/**
	 * Send the given message to the dispatcher and block until a response is received.
	 */
	InputMessage sendMessageBlocking(const OutputMessage& msg) {
		assert( msg.getHeader().isRequestWithReturn() );
		assert(msg.getClientIdentifier() == 0);

#ifdef ENABLE_TRAFFIC_LOGGING
		log_verbose( "Send blocking message : %s", msg.toString().c_str() );
#endif

		const IPCMessage& ipcMessage = msg.getIPCMessage();

		std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex);    // we prevent other threads to steal the response of our request
		writeMessage(ipcMessage);

		return waitForAnswer(msg);
	}

	/**
	 * Returns a dump of the daemon's internal state, which can be useful for diagnostic.
	 */
	std::string getDaemonStateDump() {
		IPCOutputMessage msg(IPCMessageType::DUMP_STATE);
		IPCInputMessage returnMessage = writeRequest(msg);

		if (returnMessage.getReturnCode() == IPCReturnCode::ERROR)
			throw Exception("Can't dump state");

		IPCInputMessageReader reader(returnMessage);

		std::string s;
		reader >> s;

		return s;
	}

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

	InputMessage waitForAnswer(const OutputMessage& requestMsg) {
		std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex);    // we prevent other threads to steal the response of our request

		InputMessage answerMessage;

		log_debug("Waiting for answer to message : ") << requestMsg;

		readIncomingMessagesBlocking([&] (IPCInputMessage & incomingIPCMsg) {

						     if (incomingIPCMsg.getMessageType() == IPCMessageType::SEND_MESSAGE) {
							     InputMessage inputMsg = readMessageFromIPCMessage(incomingIPCMsg);
							     if ( inputMsg.isAnswerTo(requestMsg) ) {

		                                                     // copy the content into the message to return to the caller
								     answerMessage.copyFrom(incomingIPCMsg);
#ifdef ENABLE_TRAFFIC_LOGGING
								     log_debug( "Answer received : %s", inputMsg.toString().c_str() );
#endif
								     return false;
							     } else {
								     pushToQueue(incomingIPCMsg);
								     return true;
							     }
						     }

						     return true;
					     });

		return answerMessage;
	}

	void pushToQueue(const IPCInputMessage& msg) {
		m_queue.push(msg);

		log_verbose( "Message pushed : %s", msg.toString().c_str() );

		if (m_mainLoop)
			m_mainLoop->addIdleCallback([&] () {
							    dispatchQueuedMessages();
						    });

	}

	IPCInputMessage writeRequest(IPCOutputMessage& ipcMessage) {
		IPCInputMessage answerMessage;
		std::lock_guard<std::recursive_mutex> emissionLock(dataEmissionMutex);
		std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex);    // we prevent other threads to steal the response of our request

		ipcMessage.assignRequestID();

		writeBlocking(ipcMessage);

		auto returnCode = readIncomingMessagesBlocking([&] (IPCInputMessage & incomingMsg) {

								       if ( incomingMsg.isResponseOf(ipcMessage) ) {
									       answerMessage = incomingMsg;
									       return false;
								       } else {
									       pushToQueue(incomingMsg);
									       return true;
								       }

							       });

		if (returnCode != IPCOperationReport::OK)
			answerMessage.setError();

		return answerMessage;
	}

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

	void dispatchQueuedMessages() {
		const IPCInputMessage* msg = NULL;
		//		log_debug("dispatchQueuedMessages");
		do {
			{
				std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex);
				msg = m_queue.pop();
			}

			if (msg != NULL) {
				handleConstIncomingIPCMessage(*msg);
				delete msg;
			}
		} while (msg != NULL);
	}

	void onIPCInputMessage(IPCInputMessage& inputMessage) {
		dispatchQueuedMessages();
		handleIncomingIPCMessage(inputMessage);
	}

	void handleIncomingIPCMessage(IPCInputMessage& inputMessage) override {
		handleConstIncomingIPCMessage(inputMessage);
	}

	void handleConstIncomingIPCMessage(const IPCInputMessage& inputMessage) {

		IPCInputMessageReader reader(inputMessage);

		auto messageType = inputMessage.getMessageType();

		switch (messageType) {

		case IPCMessageType::SEND_MESSAGE : {
			const InputMessage msg = readMessageFromIPCMessage(inputMessage);

#ifdef ENABLE_TRAFFIC_LOGGING
			log_debug( "Dispatching message %s:", msg.toString().c_str() );
#endif
			if (m_endPoint.processMessage(msg) == MessageProcessingResult::NotProcessed_OK)
				messageReceivedCallback->processMessage(msg);
		}
		break;

		case IPCMessageType::PING : {
			for (size_t i = 0; i < inputMessage.getUserDataLength(); i++) {
				uint8_t v;
				reader >> v;
				//				assert(v == 0x11);
				assert( v == static_cast<uint8_t>(i) );
			}
			IPCOutputMessage ipcMessage(IPCMessageType::PONG);
			writeMessage(ipcMessage);
			log_verbose("PING RECEIVED");
		}
		break;

		case IPCMessageType::SERVICES_REGISTERED : {
			while ( reader.remainingBytesCount() >= sizeof(SomeIP::ServiceID) ) {
				SomeIP::ServiceID serviceID;
				reader >> serviceID;
				getServiceRegistry().onServiceRegistered(serviceID);
			}
		}
		break;

		case IPCMessageType::SERVICES_UNREGISTERED : {
			while ( reader.remainingBytesCount() >= sizeof(SomeIP::ServiceID) ) {
				SomeIP::ServiceID serviceID;
				reader >> serviceID;
				getServiceRegistry().onServiceUnregistered(serviceID);
			}
		}
		break;

		default : {
			log_error( "Unknown message type : %i", static_cast<int>(messageType) );
		}
		break;
		}
	}

	void onCongestionDetected() override {
		enqueueIncomingMessages();

		// we block until we can read or write to the socket
		struct pollfd fd;
		fd.fd = getFileDescriptor();
		fd.events = POLLIN | POLLOUT | POLLHUP;
		poll(&fd, 1, 1000);

	}

	void onDisconnected() override {
		log_warning("Disconnected from server");
		messageReceivedCallback->onDisconnected();
	}

	void swapInputMessage() {
		//		log_info("swap");
		IPCInputMessage* msg = new IPCInputMessage();
		setInputMessage(*msg);
	}

	IPCOperationReport readIncomingMessagesBlocking(IPCMessageReceivedCallbackFunction dispatchFunction) {

		bool bKeepProcessing;

		do {

			IPCInputMessage* msg = NULL;

			{
				std::unique_lock<std::recursive_mutex> receptionLock(dataReceptionMutex);
				returnIfError( readBlocking(*m_currentInputMessage) );
				msg = m_currentInputMessage;
				swapInputMessage();
			}

			bKeepProcessing = dispatchFunction(*msg);

			delete msg;

		} while (bKeepProcessing);

		return IPCOperationReport::OK;
	}

	bool readIncomingMessages(IPCMessageReceivedCallbackFunction dispatchFunction) {

		bool bKeepProcessing = true;

		do {

			IPCInputMessage* msg = NULL;

			{
				std::unique_lock<std::recursive_mutex> receptionLock(dataReceptionMutex);
				readNonBlocking(*m_currentInputMessage);
				if ( m_currentInputMessage->isComplete() ) {
					msg = m_currentInputMessage;
					swapInputMessage();
				}
			}

			if (msg != NULL) {
				bKeepProcessing = dispatchFunction(*msg);
				delete msg;

			} else
				bKeepProcessing = false;

		} while (bKeepProcessing);

		return true;
	}

	void onConnectionLost() {
		log_error("Connection to daemon lost");
	}

	ClientConnectionListener* messageReceivedCallback = NULL;

	//	friend class GLibIntegration;

	std::vector<OutputMessageWithReport> m_messagesWithReport;

	MainLoopInterface* m_mainLoop = NULL;

	ServiceRegistry m_registry;

	SafeMessageQueue m_queue;

	SomeIPEndPoint m_endPoint;

	std::recursive_mutex dataReceptionMutex;
	std::recursive_mutex dataEmissionMutex;

};


//class GLibIntegration;

//GLibIntegration& defaultContextGlibIntegrationSingleton(std::function<void()> timerFunction);

/**
 * This class integrates the dispatching of messages into a Glib main loop.
 */
class GLibIntegration : public GlibChannelListener, private ClientConnection::MainLoopInterface {

	LOG_SET_CLASS_CONTEXT(clientLibContext);

public:
	GLibIntegration(ClientConnection& connection, GMainContext* context = NULL) :
		m_connection(connection), m_timer([&]() {
		                                          //							  m_connection.dispatchQueuedMessages();
						  }, 5000), m_channelWatcher(*this, context), m_idleCallBack(
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

	GLibTimer m_timer;

	GlibChannelWatcher m_channelWatcher;

	GlibIdleCallback m_idleCallBack;
};

}
