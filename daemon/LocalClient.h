#pragma once

#include "Dispatcher.h"

#include "ipc.h"
#include "SomeIP-common.h"
#include "GlibIO.h"

#include "ipc/UDSConnection.h"

namespace SomeIP_Dispatcher {

class LocalServer;

/**
 * Represents a local client connected via local IPC
 */
class LocalClient : public Client, public ServiceRegistrationListener, private UDSConnection {

	using SocketStreamConnection::disconnect;

	void disconnect() {
		SocketStreamConnection::disconnect();
	}

protected:
	LOG_DECLARE_CLASS_CONTEXT("LoCl", "LocalClient");

public:
	using SocketStreamConnection::isConnected;

	LocalClient(Dispatcher& dispatcher, int fd, MainLoopContext& context) :
		Client(dispatcher), m_mainLoopContext(context) {
		setInputMessage(m_inputMessage);
		setFileDescriptor(fd);
	}

	~LocalClient() {
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

	void initConnection();

	/**
	 * Send the service registry
	 */
	void sendRegistry() {
		IPCOutputMessage msg(IPCMessageType::SERVICES_REGISTERED);
		for ( auto& service : getDispatcher().getServices() )
			msg << service->getServiceIDs().serviceID << service->getServiceIDs().instanceID ;

		writeNonBlocking(msg);
	}

	void onNotificationSubscribed(Service& serviceID, SomeIP::MemberID memberID) override {
		// TODO : send message to inform the client that we are interested in the notification
	}

	SomeIPReturnCode sendMessage(const DispatcherMessage& msg) override {
		log_traffic() << "Sending message to client " << toString() << ". Message: " << msg.toString();
		return !isError(sendIPCMessage( msg.getIPCMessage() )) ? SomeIPReturnCode::OK : SomeIPReturnCode::ERROR;
	}

	SomeIPReturnCode sendMessage(const OutputMessage& msg) override {
		log_traffic() << "Sending message to client " << toString() << ". Message: " << msg.toString();
		return sendIPCMessage( msg.getIPCMessage() );
	}

	InputMessage sendMessageBlocking(const OutputMessage& msg) override {
		sendMessage(msg);
		assert(false);
	}

	SomeIPReturnCode sendIPCMessage(const IPCMessage& msg) {
		writeNonBlocking(msg);
		return SomeIPReturnCode::OK;
	}

	void onServiceRegistered(const Service& service) override {
		if ( isConnected() ) {
			IPCOutputMessage msg(IPCMessageType::SERVICES_REGISTERED);
			msg << service.getServiceIDs().serviceID << service.getServiceIDs().instanceID;
			writeNonBlocking(msg);
		}
	}

	void onServiceUnregistered(const Service& service) override {
		if ( isConnected() ) {
			IPCOutputMessage msg(IPCMessageType::SERVICES_UNREGISTERED);
			msg << service.getServiceIDs().serviceID << service.getServiceIDs().instanceID;
			writeNonBlocking(msg);
		}
	}

	void handleIncomingIPCMessage(IPCInputMessage& inputMessage) override;

	void sendPingMessage();

	std::string toString() const {
		char buffer[1000];
		snprintf( buffer, sizeof(buffer), "Client :pid %i name:%s id:%i", pid, processName.c_str(), getIdentifier() );
		return buffer;
	}

	void onDisconnected() override {
		getDispatcher().removeServiceRegistrationListener(*this);
		unregisterClient();

		m_inputDataWatcher->disable();
		m_outputDataWatcher->disable();
		m_disconnectionWatcher->disable();
	}

	void onCongestionDetected() override {
		m_outputDataWatcher->enable();
		log_info() << "Congestion with " << toString();
	}

	/**
	 * Called whenever some data is received from a client
	 */
	WatchStatus onIncomingDataAvailable();

	WatchStatus onWritingPossible() {
		return (writePendingDataNonBlocking() ==
			IPCOperationReport::OK) ? WatchStatus::STOP_WATCHING : WatchStatus::KEEP_WATCHING;
	}

private:
	IPCInputMessage m_inputMessage;

	std::unique_ptr<WatchMainLoopHook> m_inputDataWatcher;
	std::unique_ptr<WatchMainLoopHook> m_outputDataWatcher;
	std::unique_ptr<WatchMainLoopHook> m_disconnectionWatcher;

	MainLoopContext& m_mainLoopContext;

	/// PID of the client process
	pid_t pid = -1;

	/// A textual representation of the process, used for logging
	std::string processName;

};

}
