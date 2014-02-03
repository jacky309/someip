#pragma once

#include "Dispatcher.h"

#include "ipc.h"
#include "pelagicore-common.h"
#include "GlibIO.h"

#include "ipc/UDSConnection.h"

class LocalServer;

/**
 * Represents a local client connected via local IPC
 */
class LocalClient : public Client, private GlibChannelListener, public ServiceRegistrationListener, private UDSConnection {

	using SocketStreamConnection::disconnect;

	void disconnect() {
		SocketStreamConnection::disconnect();
	}

protected:
	LOG_DECLARE_CLASS_CONTEXT("LoCl", "LocalClient");

public:
	using SocketStreamConnection::isConnected;

	LocalClient(Dispatcher& dispatcher, int fd) :
		Client(dispatcher), m_channelWatcher(*this) {
		setInputMessage(m_inputMessage);
		setFileDescriptor(fd);
	}

	bool isConnected() const override {
		return SocketStreamConnection::isConnected();
	}

	void init() override {
		initConnection();
	}

	void registerClient() {
		Client::registerClient();
		getDispatcher().addServiceRegistrationListener(*this);
	}

	void initConnection() {
		pid = getPidFromFiledescriptor( getFileDescriptor() );
		processName = getProcessName(pid);
		m_channelWatcher.setup( getFileDescriptor() );
		m_channelWatcher.enableWatch();
		sendPingMessage();
		sendRegistry();
	}

	~LocalClient() override {
	}

	/**
	 * Send the service registry
	 */
	void sendRegistry() {
		IPCOutputMessage msg(IPCMessageType::SERVICES_REGISTERED);
		for ( auto& service : getDispatcher().getServices() )
			msg << service->getServiceID();

		writeNonBlocking(msg);
	}

	void onNotificationSubscribed(SomeIP::MemberID serviceID, SomeIP::MemberID memberID) override {
		// TODO : send message to inform the client that we are interested in the notification
	}

	void sendMessage(const DispatcherMessage& msg) override {

#ifdef ENABLE_TRAFFIC_LOGGING
		log_debug( "Sending message to client %s. Message: %s", toString().c_str(), msg.toString().c_str() );
#endif

		sendIPCMessage( msg.getIPCMessage() );
	}

	SomeIPFunctionReturnCode sendMessage(OutputMessage& msg) override {

#ifdef ENABLE_TRAFFIC_LOGGING
		log_debug( "Sending message to client %s. Message: %s", toString().c_str(), msg.toString().c_str() );
#endif

		return sendIPCMessage( msg.getIPCMessage() );
	}

	SomeIPFunctionReturnCode sendIPCMessage(const IPCMessage& msg) {
		writeNonBlocking(msg);
		return SomeIPFunctionReturnCode::OK;
	}

	void onServiceRegistered(const Service& service) override {
		IPCOutputMessage msg(IPCMessageType::SERVICES_REGISTERED);
		msg << service.getServiceID();
		writeNonBlocking(msg);
	}

	void onServiceUnregistered(const Service& service) override {
		IPCOutputMessage msg(IPCMessageType::SERVICES_UNREGISTERED);
		msg << service.getServiceID();
		writeNonBlocking(msg);
	}

	void handleIncomingIPCMessage(IPCInputMessage& inputMessage) override {
		IPCInputMessageReader reader(inputMessage);

		switch ( inputMessage.getMessageType() ) {

		case IPCMessageType::SEND_MESSAGE : {
			DispatcherMessage msg = readMessageFromIPCMessage(inputMessage);
			processIncomingMessage(msg);
		}
		break;

		case IPCMessageType::REGISTER_SERVICE : {

			SomeIP::ServiceID serviceID;
			reader >> serviceID;

			log_debug("REGISTER_SERVICE Message received from client %s. ServiceID:0x%X",
				  toString().c_str(), serviceID);
			Service* service = registerService(serviceID, true);

			sendPingMessage();

			IPCReturnCode returnCode = (service != NULL) ? IPCReturnCode::OK : IPCReturnCode::ERROR;

			IPCOutputMessage answer(inputMessage, returnCode);
			writeNonBlocking(answer);
		}
		break;

		case IPCMessageType::UNREGISTER_SERVICE : {

			SomeIP::ServiceID serviceID;
			reader >> serviceID;

			log_debug("UNREGISTER_SERVICE Message received from client %s. ServiceID:0x%X",
				  toString().c_str(), serviceID);

			IPCReturnCode returnCode = IPCReturnCode::ERROR;

			for (auto service : m_registeredServices) {
				if (service->getServiceID() == serviceID) {
					getDispatcher().unregisterService(*service);
					removeFromVector(m_registeredServices, service);
					returnCode = IPCReturnCode::OK;
					break;
				}
			}

			IPCOutputMessage answer(inputMessage, returnCode);
			writeNonBlocking(answer);
		}
		break;

		case IPCMessageType::PONG : {
			log_debug( "PONG Message received from client %s", toString().c_str() );
			//			sendTestMessage();
		}
		break;

		case IPCMessageType::SUBSCRIBE_NOTIFICATION : {
			SomeIP::MessageID messageID;
			reader >> messageID;
			subscribeToNotification(messageID);
		}
		break;

		case IPCMessageType::DUMP_STATE : {

			log_debug("DUMP_STATE Message received from client") << toString();

			IPCOutputMessage answer(inputMessage, IPCReturnCode::OK);
			auto str = getDispatcher().dumpState();
			answer << str;
			writeNonBlocking(answer);

		}
		break;

		default : {
			log_error( "Unknown message type : %s", ::toString( inputMessage.getMessageType() ).c_str() );
		}
		break;
		}

	}

	void sendPingMessage() {
#ifdef ENABLE_PING
		log_verbose( "Sending PING message to %s", toString().c_str() );
		IPCOutputMessage msg(IPCMessageType::PING);
		for (size_t i = 0; i < 256; i++) {
			uint8_t v = i;
			msg << v;
		}

		writeNonBlocking(msg);
#endif
	}

	std::string toString() const {
		char buffer[1000];
		snprintf( buffer, sizeof(buffer), "Client :pid %i name:%s id:%i", pid, processName.c_str(), getIdentifier() );
		return buffer;
	}

	void onDisconnected() override {
		getDispatcher().removeServiceRegistrationListener(*this);
		unregisterClient();

		m_channelWatcher.disableWatch();
	}

	void onCongestionDetected() override {
		m_channelWatcher.enableOutputWatch();
		log_info( "Congestion %s", toString().c_str() );
	}

	/**
	 * Called whenever some data is received from a client
	 */
	WatchStatus onIncomingDataAvailable() override {

		if (inputBlocked) {
			return WatchStatus::STOP_WATCHING;
		}

		bool bKeepProcessing = true;

		do {
			readNonBlocking(*m_currentInputMessage);

			IPCInputMessage& inputMessage = *m_currentInputMessage;

			if ( inputMessage.isComplete() ) {
				handleIncomingIPCMessage(inputMessage);
				setInputMessage(inputMessage);
			} else
				bKeepProcessing = false;

		} while (bKeepProcessing);

		return WatchStatus::KEEP_WATCHING;

	}

	WatchStatus onWritingPossible() override {
		return (writePendingDataNonBlocking() ==
			IPCOperationReport::OK) ? WatchStatus::STOP_WATCHING : WatchStatus::KEEP_WATCHING;
	}

private:
	IPCInputMessage m_inputMessage;

	GlibChannelWatcher m_channelWatcher;

	/// PID of the client process
	pid_t pid = -1;

	/// A textual representation of the process, used for logging
	std::string processName;

};
