#pragma once

#include <netdb.h>

#include "SomeIP-common.h"
#include <netinet/tcp.h>

#include "Client.h"
#include "SocketStreamConnection.h"
#include "ServiceDiscovery.h"

namespace SomeIP_Dispatcher {

using namespace SomeIP_utils;

class TCPManager;
class TCPServer;

/**
 * Handles a client connected via TCP
 */
class TCPClient : public Client,
	protected SocketStreamConnection,
	private ServiceDiscoveryListener {

protected:
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
	TCPClient(Dispatcher& dispatcher, TCPManager& tcpManager, MainLoopContext& mainContext, ServiceInstanceNamespace& instancesNamespace, int fileDescriptor = UNINITIALIZED_FILE_DESCRIPTOR) :
		Client(dispatcher), m_tcpManager(tcpManager), m_serviceDiscoveryDecoder(*this), m_mainLoopContext(mainContext), m_instanceNamespace(instancesNamespace) {
		setFileDescriptor(fileDescriptor);
	}

	~TCPClient() {
	}

	void init() override {
		if (getFileDescriptor() != UNINITIALIZED_FILE_DESCRIPTOR)
			setupConnection();
	}

	void onRemoteClientSubscription(const SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
					const IPv4ConfigurationOption* address) override {

		// TODO : consider the address
		assert(address == nullptr);
		subscribeToNotification( MemberIDs(serviceEntry.m_serviceID, serviceEntry.m_instanceID, serviceEntry.m_eventGroupID) );
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
	}

	void setupConnection() {
		m_headerReader.setBuffer( m_headerBytes, sizeof(m_headerBytes) );

		{
			pollfd fd;
			fd.fd = getFileDescriptor();
			fd.events = POLLIN;

			m_inputDataWatcher = m_mainLoopContext.addFileDescriptorWatch([&] () {
										return processIncomingData(getFileDescriptor(), [&] (InputMessage& msg) {
											auto serviceID = msg.getServiceID();

//											assert(m_instanceNamespace != nullptr);
											log_debug() << (size_t) &m_instanceNamespace;
											log_debug() << m_instanceNamespace;

											assert(m_instanceNamespace.count(serviceID) == 1);

											auto service = m_instanceNamespace.at(serviceID);
											msg.setInstanceID(service->getServiceIDs().instanceID);

											processIncomingMessage(msg);
										});
									}, fd);
			m_inputDataWatcher->enable();
		}

		{
			pollfd fd;
			fd.fd = getFileDescriptor();
			fd.events = POLLOUT;
			m_outputDataWatcher = m_mainLoopContext.addFileDescriptorWatch([&] () {
										 return onWritingPossible();
									 }, fd);
		}

		{
			pollfd fd;
			fd.fd = getFileDescriptor();
			fd.events = POLLHUP;
			m_disconnectionWatcher = m_mainLoopContext.addFileDescriptorWatch([&] () {
										    disconnect();
									    }, fd);
			m_disconnectionWatcher->enable();
		}

	}

	void onServiceAvailable(ServiceIDs serviceID) {
		Service* service = registerService(serviceID, false);
		assert( (m_instanceNamespace.count(serviceID.serviceID) == 0) ||
				( m_instanceNamespace.at(serviceID.serviceID)->getServiceIDs().instanceID == serviceID.instanceID));
		m_instanceNamespace[serviceID.serviceID] = service;
		log_debug() << m_instanceNamespace;
	}

	void onServiceUnavailable(ServiceIDs serviceID) {
		unregisterService(serviceID);
//		assert(m_instanceNamespace.count(serviceID.serviceID) == 1);
		m_instanceNamespace.erase(serviceID.serviceID);
		log_debug() << m_instanceNamespace;
	}

	void onDisconnected() override {
		unregisterClient();
		m_inputDataWatcher->disable();
		m_outputDataWatcher->disable();
		m_disconnectionWatcher->disable();
	}

	static void enableNoDelay(int fd) {
		// minimize latency by ensuring that the data is always sent immediately
		int on = 1;
		if ( setsockopt( fd, SOL_SOCKET, TCP_NODELAY, &on, sizeof(on) ) )
			log_error() << "Can't enable TCP_NODELAY on the socket. Error : " << strerror(errno);
	}

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

	InputMessage sendMessageBlocking(const OutputMessage& msg) override {

		InputMessage answer;

		sendMessage(msg);

		bool responseReceived = false;

		while(!responseReceived)
			processIncomingData(getFileDescriptor(), [&] (InputMessage& inputMessage) {
				if (inputMessage.isAnswerTo(msg)) {
					answer = inputMessage;
					responseReceived = true;
				}
				else {
					log_error() << "received : " << inputMessage;
					m_messageQueue.push_back(inputMessage);
//					assert(false);
				}
			});

		return answer;
	}

	virtual SomeIPReturnCode connect() {
		return SomeIPReturnCode::OK;
	}

	SomeIPReturnCode sendMessage(const OutputMessage& msg) override {

		if ( !isConnected() )
			connect();

		log_traffic( ) << "Sending message to client " << toString() << " Message: " << msg.toString();

		return !isError(sendMessage( msg.getHeader(), msg.getPayload(), msg.getPayloadLength() )) ? SomeIPReturnCode::OK : SomeIPReturnCode::ERROR;
	}

	IPCOperationReport sendMessage(const SomeIP::SomeIPHeader& header, const void* payload, size_t payloadLength);

	void onNotificationSubscribed(Service& serviceID, SomeIP::MemberID memberID) override;

	void onCongestionDetected() override {
		m_outputDataWatcher->enable();
		log_info() << "Congestion " << toString();
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
	WatchStatus processIncomingData(int fileDescriptor, std::function<void(InputMessage&)> handler);

	WatchStatus onWritingPossible() {
		return (writePendingDataNonBlocking() ==
			IPCOperationReport::OK) ? WatchStatus::STOP_WATCHING : WatchStatus::KEEP_WATCHING;
	}

	std::string toString() const {
		return StringBuilder() << "TCP Client fd:" << getFileDescriptor();
	}

	bool detectReboot(const SomeIP::SomeIPServiceDiscoveryHeader& serviceDiscoveryHeader, bool isMulticast) {
		RebootInformation& info = (isMulticast ? m_rebootInformationMulticast : m_rebootInformationUnicast);
		return info.updateAndTestReboot(serviceDiscoveryHeader);
	}

private:

	MyInputMessage m_currentIncomingMessage;

	unsigned char m_headerBytes[SomeIP::SOMEIP_HEADER_LENGTH_ON_NETWORK];
	IPCBufferReader m_headerReader;
	IPCBufferReader m_payloadReader;

	TCPManager& m_tcpManager;
	ServiceDiscoveryMessageDecoder m_serviceDiscoveryDecoder;

	MainLoopContext& m_mainLoopContext;

	std::unique_ptr<WatchMainLoopHook> m_inputDataWatcher;
	std::unique_ptr<WatchMainLoopHook> m_outputDataWatcher;
	std::unique_ptr<WatchMainLoopHook> m_disconnectionWatcher;

	RebootInformation m_rebootInformationMulticast;
	RebootInformation m_rebootInformationUnicast;

	TCPServer* m_tcpServer = nullptr;

	ServiceInstanceNamespace& m_instanceNamespace;

	std::vector<InputMessage> m_messageQueue;

};


class RemoteTCPClient : public TCPClient {

public:

	/**
	 * Constructor used to register a host offering one or several services
	 */
	RemoteTCPClient(Dispatcher& dispatcher, TCPManager& tcpManager, MainLoopContext& mainContext, IPv4TCPEndPoint server) :
		TCPClient(dispatcher, tcpManager, mainContext, m_ownInstanceNamespace) , m_serverIdentifier(server) {
	}

	IPv4TCPEndPoint& getServerID() {
		return m_serverIdentifier;
	}

	std::string toString() const {
		return StringBuilder() << TCPClient::toString() << "with address: " << m_serverIdentifier.toString();
	}

	SomeIPReturnCode connect() override;

private:
	ServiceInstanceNamespace m_ownInstanceNamespace;
	IPv4TCPEndPoint m_serverIdentifier;

};

}
