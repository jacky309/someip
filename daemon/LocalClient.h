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
			msg << service->getServiceID();

		writeNonBlocking(msg);
	}

	void onNotificationSubscribed(SomeIP::MemberID serviceID, SomeIP::MemberID memberID) override {
		// TODO : send message to inform the client that we are interested in the notification
	}

	void sendMessage(const DispatcherMessage& msg) override {
		log_traffic( "Sending message to client %s. Message: %s", toString().c_str(), msg.toString().c_str() );
		sendIPCMessage( msg.getIPCMessage() );
	}

	SomeIPReturnCode sendMessage(OutputMessage& msg) override {
		log_traffic( "Sending message to client %s. Message: %s", toString().c_str(), msg.toString().c_str() );
		return sendIPCMessage( msg.getIPCMessage() );
	}

	SomeIPReturnCode sendIPCMessage(const IPCMessage& msg) {
		writeNonBlocking(msg);
		return SomeIPReturnCode::OK;
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

		m_channelWatcher.disableWatch();
	}

	void onCongestionDetected() override {
		m_channelWatcher.enableOutputWatch();
		log_info( "Congestion %s", toString().c_str() );
	}

	/**
	 * Called whenever some data is received from a client
	 */
	WatchStatus onIncomingDataAvailable() override;

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

}
