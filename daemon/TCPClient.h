#pragma once

#include <netdb.h>

#include "SomeIP-common.h"
#include <netinet/tcp.h>

#include "Client.h"
#include "SocketStreamConnection.h"

namespace SomeIP_Dispatcher {

class TCPManager;

/**
 * Handles a client connected via TCP
 */
class TCPClient : public Client,
	private SocketStreamConnection,
	private GlibChannelListener,
	private ServiceDiscoveryListener {

	LOG_DECLARE_CLASS_CONTEXT("TCPC", "TCPClient");

	struct RebootInformation {

		bool updateAndTestReboot(const SomeIP::SomeIPServiceDiscoveryHeader& serviceDiscoveryHeader) {
			bool hasRebooted =
				( serviceDiscoveryHeader.getRebootFlag() &&
				  (serviceDiscoveryHeader.getRequestID() < m_lastReceivedSessionID) );
			m_lastReceivedSessionID = serviceDiscoveryHeader.getRequestID();
			return hasRebooted;
		}

		SomeIP::RequestID m_lastReceivedSessionID = 0;

	};

public:
	using SocketStreamConnection::disconnect;
	using SocketStreamConnection::isConnected;

	void disconnect() {
		SocketStreamConnection::disconnect();
	}

	bool isConnected() const override {
		return SocketStreamConnection::isConnected();
	}

	/**
	 * Constructor used when a remote client connects to us
	 */
	TCPClient(Dispatcher& dispatcher, int fileDescriptor, TCPManager& tcpManager) :
		Client(dispatcher), m_tcpManager(tcpManager), m_serviceDiscoveryDecoder(*this), m_channelWatcher(*this) {
		setFileDescriptor(fileDescriptor);
	}

	/**
	 * Constructor used to register a host offering one or several services
	 */
	TCPClient(Dispatcher& dispatcher, IPv4TCPServerIdentifier server, TCPManager& tcpManager) :
		Client(dispatcher), ServiceDiscoveryListener(*this), m_tcpManager(tcpManager), m_serviceDiscoveryDecoder(
			*this), m_channelWatcher(*this) {
		m_serverIdentifier = server;
	}

	~TCPClient() override {
	}

	void init() override {
		if (getFileDescriptor() != UNINITIALIZED_FILE_DESCRIPTOR)
			setupConnection();
	}

	void onRemoteClientSubscription(const SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
					const IPv4ConfigurationOption* address) override {

		// TODO : consider the address
		assert(address == NULL);
		subscribeToNotification( SomeIP::getMessageID(serviceEntry.m_serviceID, serviceEntry.m_eventGroupID) );
	}

	void onRemoteClientSubscriptionFinished(const SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
						const IPv4ConfigurationOption* address) override {
		// TODO : implement
	}

	void onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const IPv4ConfigurationOption* address,
				      const SomeIPServiceDiscoveryMessage& message) override;

	void onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const IPv4ConfigurationOption* address,
					const SomeIPServiceDiscoveryMessage& message) override;

	void onFindServiceRequested(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				    const SomeIPServiceDiscoveryMessage& message) override {
	};

	void setupConnection() {
		m_headerReader.setBuffer( m_headerBytes, sizeof(m_headerBytes) );
		m_channelWatcher.setup( getFileDescriptor() );
		m_channelWatcher.enableWatch();
	}

	IPv4TCPServerIdentifier& getServerID() {
		return m_serverIdentifier;
	}

	void onServiceAvailable(SomeIP::ServiceID serviceID) {
		registerService(serviceID, false);
	}

	void onServiceUnavailable(SomeIP::ServiceID serviceID) {
		unregisterService(serviceID);
	}

	void onRebootDetected() {
		disconnect();
		unregisterClient();             // TODO: make sure the instance won't be destroyed by the dispatcher

		registerClient();
	}

	void onDisconnected() override {
		unregisterClient();
		m_channelWatcher.disableWatch();
	}

	static void enableNoDelay(int fd) {
		// minimize latency by ensuring that the data is always sent immediately
		int on = 1;
		if ( setsockopt( fd, SOL_SOCKET, TCP_NODELAY, &on, sizeof(on) ) )
			log_error( "Can't enable TCP_NODELAY on the socket. Error : %s", strerror(errno) );
	}

	void connect();

	void tagClientIdentifier(SomeIP::SomeIPHeader& header, ClientIdentifier clientIdentifier) const {
		SomeIP::RequestID requestID = clientIdentifier;
		requestID <<= 16;
		requestID += (header.getRequestID() & 0xFFFF);
		header.setRequestID(requestID);
	}

	ClientIdentifier extractClientIdentifier(const SomeIP::SomeIPHeader& header) const {
		return header.getRequestID() >> 16;
	}

	void sendMessage(const DispatcherMessage& msg) override;

	SomeIPReturnCode sendMessage(OutputMessage& msg) override {

		log_traffic( "Sending message to client %s. Message: %s", toString().c_str(), msg.toString().c_str() );

		sendMessage( msg.getHeader(), msg.getPayload(), msg.getPayloadLength() );

		// TODO : return proper error code if needed
		return SomeIPReturnCode::OK;
	}

	IPCOperationReport sendMessage(const SomeIP::SomeIPHeader& header, const void* payload, size_t payloadLength);

	void onNotificationSubscribed(SomeIP::MemberID serviceID, SomeIP::MemberID memberID) override;

	void onCongestionDetected() override {
		m_channelWatcher.enableOutputWatch();
		log_info() << "Congestion %s" << toString();
	}

	class MyInputMessage : public DispatcherMessage {

public:
		MyInputMessage() :
			InputMessage(m_ipcMessage) {
			m_ipcMessage.getHeader().m_messageType = IPCMessageType::SEND_MESSAGE;
		}

		SomeIP::SomeIPHeader& getHeaderPrivate() {
			return InputMessage::getHeaderPrivate();
		}

		void setPayloadSize(size_t size) {
			auto s = size + m_ipcMessage.getHeaderSize() + getHeaderSize();
			m_ipcMessage.getPayload().resize(s);
		}

		void clear() {
		}

		void* getWritablePayload() {
			return const_cast<void*>( getPayload() );
		}

private:
		IPCInputMessage m_ipcMessage;

	};

	/**
	 * Called whenever some data is received from a client
	 */
	WatchStatus onIncomingDataAvailable() override;

	WatchStatus onWritingPossible() override {
		return (writePendingDataNonBlocking() ==
			IPCOperationReport::OK) ? WatchStatus::STOP_WATCHING : WatchStatus::KEEP_WATCHING;
	}

	std::string toString() const {
		char buffer[1000];
		snprintf( buffer, sizeof(buffer), "TCP Client :address:%s", m_serverIdentifier.toString().c_str() );
		return buffer;
	}

	bool detectReboot(const SomeIP::SomeIPServiceDiscoveryHeader& serviceDiscoveryHeader, bool isMulticast) {
		RebootInformation& info = (isMulticast ? m_rebootInformationMulticast : m_rebootInformationUnicast);
		return info.updateAndTestReboot(serviceDiscoveryHeader);
	}

private:
	IPv4TCPServerIdentifier m_serverIdentifier;

	MyInputMessage m_currentIncomingMessage;

	unsigned char m_headerBytes[SomeIP::SOMEIP_HEADER_LENGTH_ON_NETWORK];
	IPCBufferReader m_headerReader;
	IPCBufferReader m_payloadReader;

	TCPManager& m_tcpManager;
	ServiceDiscoveryMessageDecoder m_serviceDiscoveryDecoder;

	GlibChannelWatcher m_channelWatcher;

	RebootInformation m_rebootInformationMulticast;
	RebootInformation m_rebootInformationUnicast;

};

}
