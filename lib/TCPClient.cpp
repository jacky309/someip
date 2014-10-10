#include "TCPClient.h"
#include "TCPManager.h"

namespace SomeIP_Dispatcher {

/**
 * Called whenever some data is received from a client
 */
WatchStatus TCPClient::processIncomingData(int fileDescriptor, std::function<void(InputMessage&)> handler) {

	if( isInputBlocked() )
		return WatchStatus::STOP_WATCHING;

	bool bKeepProcessing = true;

	do {

		if ( !m_headerReader.isComplete() ) {

			m_headerReader.read(fileDescriptor, false);
			if ( m_headerReader.isComplete() ) {
				NetworkDeserializer deserializer( m_headerBytes, sizeof(m_headerBytes) );
				SomeIP::SomeIPHeader& header = m_currentIncomingMessage.getHeaderPrivate();
				uint32_t messageLength;
				deserializer >> header.m_messageID >> messageLength >> header.m_requestID >>
				header.m_protocolVersion
				>> header.m_interfaceVersion;
				deserializer.readEnum(header.m_messageType);
				deserializer.readEnum(header.m_returnCode);

				m_currentIncomingMessage.setPayloadSize(
					messageLength - SomeIP::SOMEIP_HEADER_LENGTH_ON_NETWORK + sizeof(SomeIP::MessageID)
					+ sizeof(messageLength) );

				m_payloadReader.setBuffer( m_currentIncomingMessage.getWritablePayload(),
							   m_currentIncomingMessage.getPayloadLength() );
			} else
				bKeepProcessing = false;

		} else {

			// We are receiving some payload
			m_payloadReader.read(fileDescriptor, false);

			if ( m_payloadReader.isComplete() ) {

				auto& header = m_currentIncomingMessage.getHeader();
				if (header.getMessageID() ==
				    SomeIPServiceDiscoveryMessage::SERVICE_DISCOVERY_MEMBER_ID) {
					m_serviceDiscoveryDecoder.decodeMessage( m_currentIncomingMessage.getHeader(),
										 m_currentIncomingMessage.getPayload(),
										 m_currentIncomingMessage.getPayloadLength() );
				} else {

					if ( header.isReply() ) {
						// This is the answer to a request => extract the client identifier from the requestID
						ClientIdentifier clientIdentifier = extractClientIdentifier(header);
						m_currentIncomingMessage.setClientIdentifier(clientIdentifier);
						tagClientIdentifier(m_currentIncomingMessage.getHeaderPrivate(), 0); // remove the client identifier from the requestID
					}

					handler(m_currentIncomingMessage);

				}
				m_headerReader.clear();
				m_payloadReader.clear();
			} else
				bKeepProcessing = false;

		}

	} while (bKeepProcessing);

	return WatchStatus::KEEP_WATCHING;
}

void TCPClient::onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					 const IPv4ConfigurationOption* address,
					 const SomeIPServiceDiscoveryMessage& message) {
	m_tcpManager.onRemoteServiceAvailable(serviceEntry, address, message);
}

void TCPClient::onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					   const IPv4ConfigurationOption* address,
					   const SomeIPServiceDiscoveryMessage& message) {
	m_tcpManager.onRemoteServiceUnavailable(serviceEntry, address, message);
}

SomeIPReturnCode RemoteTCPClient::connect() {

	int fileDescriptor;

	//		struct hostent *ptrh; /* pointer to a host table entry       */
	struct protoent* ptrp; /* pointer to a protocol table entry   */
	struct sockaddr_in sad; /* structure to hold an IP address     */
	//		char *host; /* pointer to host name                */

	memset( &sad, 0, sizeof(sad) ); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet     */
	sad.sin_port = htons(m_serverIdentifier.m_port);

	auto addr = m_serverIdentifier.m_address.getInAddr();
	memcpy( &(sad.sin_addr), &addr, sizeof(sad.sin_addr) );

	/* Map TCP transport protocol name to protocol number. */
	if ( ( ptrp = getprotobyname("tcp") ) == 0 ) {
		log_error() << "Failed to connect to server. Cannot map \"tcp\" to protocol number";
		return SomeIPReturnCode::ERROR;
	}

	/* Create a socket. */
	fileDescriptor = socket(AF_INET, SOCK_STREAM, ptrp->p_proto);
	if (fileDescriptor < 0) {
		log_error( ) << "Failed to connect to server. Socket creation failed. Error : " << strerror(errno);
		return SomeIPReturnCode::ERROR;
	}

	enableNoDelay(fileDescriptor);

	log_debug() << "Connecting to " << m_serverIdentifier.toString();

	/* Connect the socket to the specified server. */
	if (::connect( fileDescriptor, (struct sockaddr*) &sad, sizeof(sad) ) < 0) {
		log_error( ) << "Failed to connect to server : " << m_serverIdentifier.toString();
		return SomeIPReturnCode::ERROR;
	}

	setFileDescriptor(fileDescriptor);

	setupConnection();

	return SomeIPReturnCode::OK;
}


void TCPClient::sendMessage(const DispatcherMessage& msg) {

	log_traffic() << "Sending message to client " << toString() << ". Message: " << msg.toString();

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

IPCOperationReport TCPClient::sendMessage(const SomeIP::SomeIPHeader& header, const void* payload, size_t payloadLength) {

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

void TCPClient::onNotificationSubscribed(Service& service, SomeIP::MemberID memberID) {

	auto& instances = m_instanceNamespace;

	const auto serviceID = service.getServiceIDs();

	if (instances.count(serviceID.serviceID) != 0) {
		assert(	instances[serviceID.serviceID]->getServiceIDs().instanceID == serviceID.instanceID ); // TODO : handle multiple instance
	}

	instances[serviceID.serviceID] = &service;

	if ( !isConnected() ) {
		connect();
	}

	SomeIPServiceDiscoveryMessage serviceDiscoveryMessage(true);
	SomeIPServiceDiscoverySubscribeNotificationEntry subscribeEntry(serviceID, memberID);
	serviceDiscoveryMessage.addEntry(subscribeEntry);

	ByteArray byteArray;
	NetworkSerializer s(byteArray);
	serviceDiscoveryMessage.serialize(s);

	writeBytesNonBlocking( byteArray.getData(), byteArray.size() );
}

}
