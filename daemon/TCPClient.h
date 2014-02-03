#pragma once

#include <netdb.h>

#include "pelagicore-common.h"
#include <netinet/tcp.h>

#include "Client.h"
#include "SocketStreamConnection.h"

class TCPManager;

/**
 * Handles a client connected via TCP
 */
class TCPClient : public Client,
	private SocketStreamConnection,
	private GlibChannelListener,
	private SomeIP::ServiceDiscoveryListener {

	LOG_DECLARE_CLASS_CONTEXT("TCPC", "TCPClient");

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
		Client(dispatcher), SomeIP::ServiceDiscoveryListener(*this), m_tcpManager(tcpManager), m_serviceDiscoveryDecoder(
			*this), m_channelWatcher(*this) {
		m_serverIdentifier = server;
	}

	void init() override {
		if (getFileDescriptor() != UNINITIALIZED_FILE_DESCRIPTOR)
			setupConnection();
	}

	void onRemoteClientSubscription(const SomeIP::SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
					const SomeIP::IPv4ConfigurationOption* address) override {

		// TODO : consider the address
		assert(address == NULL);
		subscribeToNotification( SomeIP::getMessageID(serviceEntry.m_serviceID, serviceEntry.m_eventGroupID) );
	}

	void onRemoteClientSubscriptionFinished(const SomeIP::SomeIPServiceDiscoveryEventGroupEntry& serviceEntry,
						const SomeIP::IPv4ConfigurationOption* address) override {
		// TODO : implement
	}

	void onRemoteServiceAvailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
				      const SomeIP::IPv4ConfigurationOption* address,
				      const SomeIP::SomeIPServiceDiscoveryMessage& message) override;

	void onRemoteServiceUnavailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					const SomeIP::IPv4ConfigurationOption* address,
					const SomeIP::SomeIPServiceDiscoveryMessage& message) override;

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

public:
	~TCPClient() override {
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

	void connect() {

		int fileDescriptor;

		//		struct hostent *ptrh; /* pointer to a host table entry       */
		struct protoent* ptrp; /* pointer to a protocol table entry   */
		struct sockaddr_in sad; /* structure to hold an IP address     */
		//		char *host; /* pointer to host name                */

		memset( &sad, 0, sizeof(sad) ); /* clear sockaddr structure */
		sad.sin_family = AF_INET; /* set family to Internet     */
		sad.sin_port = htons(m_serverIdentifier.m_port);

		/* Convert host name to equivalent IP address and copy to sad. */
		memcpy( &(sad.sin_addr), &m_serverIdentifier.m_address, sizeof(sad.sin_addr) );

		//		sad.sin_addr;

		/* Map TCP transport protocol name to protocol number. */
		if ( ( ptrp = getprotobyname("tcp") ) == 0 ) {
			log_error("Failed to connect to server. Cannot map \"tcp\" to protocol number");
		}

		/* Create a socket. */
		fileDescriptor = socket(AF_INET, SOCK_STREAM, ptrp->p_proto);
		if (fileDescriptor < 0) {
			log_error( "Failed to connect to server. Socket creation failed. Error : %s", strerror(errno) );
		}

		enableNoDelay(fileDescriptor);

		/* Connect the socket to the specified server. */
		if (::connect( fileDescriptor, (struct sockaddr*) &sad, sizeof(sad) ) < 0) {
			log_error( "Failed to connect to server : %s", m_serverIdentifier.toString().c_str() );
		}

		setFileDescriptor(fileDescriptor);

		setupConnection();
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

	void sendMessage(const DispatcherMessage& msg) override {

#ifdef ENABLE_TRAFFIC_LOGGING
		log_debug( "Sending message to client %s. Message: %s", toString().c_str(), msg.toString().c_str() );
#endif

		if ( !isConnected() ) {
			connect();
		}

		const SomeIP::SomeIPHeader& header = msg.getHeader();

		if ( header.isRequestWithReturn() ) {
			// If this is a request, we use the requestID to identify the client which is sending the request, in order to dispatch the answer to it
			SomeIP::SomeIPHeader requestHeader(header);
			tagClientIdentifier( requestHeader, msg.getClientIdentifier() );
			sendMessage( requestHeader, msg.getPayload(), msg.getPayloadLength() );
		} else
			sendMessage( header, msg.getPayload(), msg.getPayloadLength() );
	}

	SomeIPFunctionReturnCode sendMessage(OutputMessage& msg) override {

#ifdef ENABLE_TRAFFIC_LOGGING
		log_debug( "Sending message to client %s. Message: %s", toString().c_str(), msg.toString().c_str() );
#endif

		sendMessage( msg.getHeader(), msg.getPayload(), msg.getPayloadLength() );

		// TODO : return proper error code if needed
		return SomeIPFunctionReturnCode::OK;
	}

	IPCOperationReport sendMessage(const SomeIP::SomeIPHeader& header, const void* payload, size_t payloadLength) {

		ByteArray headerBytes;
		NetworkSerializer serializer(headerBytes);

		{
			serializer << header.m_messageID;
			LengthPlaceHolder<uint32_t, NetworkSerializer> lengthPlaceHolder(serializer);
			serializer << header.m_requestID << header.m_protocolVersion << header.m_interfaceVersion;
			serializer.writeEnum(header.m_messageType);
			serializer.writeEnum(header.m_returnCode);
			serializer.writeRawData(payload, payloadLength); // TODO : for big messages, send the content directly without making a copy
		}

		auto v = writeBytesNonBlocking( headerBytes.getData(), headerBytes.size() );
		return v;
	}

	void onNotificationSubscribed(SomeIP::MemberID serviceID, SomeIP::MemberID memberID) override {

		if ( !isConnected() ) {
			connect();
		}

		SomeIP::SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);
		SomeIP::SomeIPServiceDiscoverySubscribeNotificationEntry subscribeEntry(serviceID, memberID);
		serviceDiscoveryMessage.addEntry(subscribeEntry);

		ByteArray byteArray;
		NetworkSerializer s(byteArray);
		serviceDiscoveryMessage.serialize(s);

		writeBytesNonBlocking( byteArray.getData(), byteArray.size() );
	}

	void onCongestionDetected() override {
		m_channelWatcher.enableOutputWatch();
		log_info( "Congestion %s", toString().c_str() );
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

	IPv4TCPServerIdentifier m_serverIdentifier;

	MyInputMessage m_currentIncomingMessage;

	unsigned char m_headerBytes[SomeIP::SOMEIP_HEADER_LENGTH_ON_NETWORK];
	IPCBufferReader m_headerReader;
	IPCBufferReader m_payloadReader;

	TCPManager& m_tcpManager;
	SomeIP::ServiceDiscoveryMessageDecoder m_serviceDiscoveryDecoder;

	GlibChannelWatcher m_channelWatcher;

	struct RebootInformation {

		bool updateAndTestReboot(const SomeIP::SomeIPServiceDiscoveryHeader& serviceDiscoveryHeader) {
			bool hasRebooted =
				( serviceDiscoveryHeader.getRebootFlag() &&
				  (serviceDiscoveryHeader.getRequestID() < m_lastReceivedSessionID) );
			m_lastReceivedSessionID = serviceDiscoveryHeader.getRequestID();
			return hasRebooted;
		}

		//		bool m_lastReceivedRebootFlag;
		SomeIP::RequestID m_lastReceivedSessionID = 0;

	};

	RebootInformation m_rebootInformationMulticast;
	RebootInformation m_rebootInformationUnicast;

};
