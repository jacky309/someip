#include "TCPClient.h"
#include "TCPManager.h"

/**
 * Called whenever some data is received from a client
 */
WatchStatus TCPClient::onIncomingDataAvailable() {

	if(inputBlocked)
		return WatchStatus::STOP_WATCHING;

	bool bKeepProcessing = true;

	do {

		if ( !m_headerReader.isComplete() ) {

			m_headerReader.read(getFileDescriptor(), false);
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
			m_payloadReader.read(getFileDescriptor(), false);

			if ( m_payloadReader.isComplete() ) {

				auto& header = m_currentIncomingMessage.getHeader();
				if (header.getMessageID() ==
				    SomeIP::SomeIPServiceDiscoveryMessage::SERVICE_DISCOVERY_MEMBER_ID) {
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

					processIncomingMessage(m_currentIncomingMessage);

				}
				m_headerReader.clear();
				m_payloadReader.clear();
			} else
				bKeepProcessing = false;

		}

	} while (bKeepProcessing);

	return WatchStatus::KEEP_WATCHING;
}

void TCPClient::onRemoteServiceAvailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					 const SomeIP::IPv4ConfigurationOption* address,
					 const SomeIP::SomeIPServiceDiscoveryMessage& message) {
	m_tcpManager.onRemoteServiceAvailable(serviceEntry, address, message);
}

void TCPClient::onRemoteServiceUnavailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					   const SomeIP::IPv4ConfigurationOption* address,
					   const SomeIP::SomeIPServiceDiscoveryMessage& message) {
	m_tcpManager.onRemoteServiceUnavailable(serviceEntry, address, message);
}
