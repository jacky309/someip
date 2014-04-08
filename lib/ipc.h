#pragma once

#include <vector>
#include <stddef.h>
#include <assert.h>
#include <memory.h>
#include "utilLib/serialization.h"
#include "SomeIP-common.h"

namespace SomeIP_Lib {

class IPCOutputMessage;

using namespace SomeIP_utils;

// TODO : move this definition
enum class SomeIPFunctionReturnCode {
	OK, DISCONNECTED, ERROR
};

inline bool isError(SomeIPFunctionReturnCode code) {
	return (code != SomeIPFunctionReturnCode::OK);
}

enum class IPCOperationReport {
	OK, BUFFER_FULL, DISCONNECTED
};

enum class IPCMessageType
	: uint8_t {
	INVALID,
	PING,
	PONG,
	SEND_MESSAGE,
	REGISTER_SERVICE,
	UNREGISTER_SERVICE,
	SUBSCRIBE_NOTIFICATION,
	DUMP_STATE,
	ANSWER,
	SERVICES_REGISTERED,
	SERVICES_UNREGISTERED
};

inline std::string toString(IPCMessageType messageType) {
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, INVALID);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, PING);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, PONG);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, SEND_MESSAGE);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, REGISTER_SERVICE);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, UNREGISTER_SERVICE);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, SUBSCRIBE_NOTIFICATION);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, DUMP_STATE);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, ANSWER);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, SERVICES_REGISTERED);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, IPCMessageType, SERVICES_UNREGISTERED);
	return "Unknown value of IPCMessageType";
}

enum class IPCReturnCode
	: uint8_t {
	UNDEFINED, OK, ERROR
};

typedef uint16_t IPCRequestID;

struct IPCMessageHeader {
	IPCRequestID m_requestID;
	IPCMessageType m_messageType;
	IPCReturnCode m_returnCode;
};

/**
 * Used to manipulate
 */
class IPCMessage {
public:
	IPCMessage() {
		// reserve space for the header
		m_payload.resize( sizeof(IPCMessageHeader) );
	}

	IPCMessageType getMessageType() const {
		return getHeader().m_messageType;
	}

	IPCMessageHeader& getHeader() {
		return *reinterpret_cast<IPCMessageHeader*>( m_payload.getData() );
	}

	const IPCMessageHeader& getHeader() const {
		return *reinterpret_cast<const IPCMessageHeader*>( m_payload.getData() );
	}

	static size_t getHeaderSize() {
		return sizeof(IPCMessageHeader);
	}

	IPCRequestID getRequestID() const {
		return getHeader().m_requestID;
	}

	std::string toString() const {
		char buffer[1000];
		snprintf( buffer, sizeof(buffer), "IPCMessage:MessageType:%s payload:%s", SomeIP_Lib::toString(
				  getMessageType() ).c_str(),
			  byteArrayToString( m_payload.getData(), m_payload.size() ).c_str() );
		return buffer;
	}

	IPCReturnCode getReturnCode() {
		return getHeader().m_returnCode;
	}

	const ResizeableByteArray& getPayload() const {
		return m_payload;
	}

	ResizeableByteArray& getPayload() {
		return m_payload;
	}

	const unsigned char* getUserData() const {
		return m_payload.getData() + sizeof(IPCMessageHeader);
	}

	unsigned char* getUserData() {
		return m_payload.getData() + sizeof(IPCMessageHeader);
	}

	size_t getUserDataLength() const {
		return m_payload.size() - sizeof(IPCMessageHeader);
	}

private:
	ByteArray m_payload;

};

/**
 * An input message
 */
class IPCInputMessage : public IPCMessage {

	static const size_t UNKNOWN_SIZE = -1;

public:
	IPCInputMessage() {
		clear();
	}

	~IPCInputMessage() {
	}

	IPCInputMessage(const IPCInputMessage& msg) {
		*this = msg;
	}

	bool isComplete() const {
		return (m_totalMessageSize == m_receivedSize);
	}

	bool isLengthReceived() const {
		return (m_totalMessageSize != UNKNOWN_SIZE);
	}

	void clear() {
		getHeader().m_messageType = IPCMessageType::INVALID;
		getHeader().m_requestID = 0;
		m_receivedSize = 0;
		m_totalMessageSize = UNKNOWN_SIZE;
	}

	void setError() {
		m_isError = true;
	}

	bool isResponseOf(const IPCOutputMessage& outputMessage) const;

	size_t getLength() const {
		return m_totalMessageSize;
	}

	void setLength(size_t length) {
		getPayload().resize(length);
		m_totalMessageSize = length;
		m_receivedSize = 0;
	}

	size_t getReceivedSize() {
		return m_receivedSize;
	}

private:
	size_t m_totalMessageSize;
	size_t m_receivedSize;

	bool m_isError = false;

	friend class UDSConnection;
};

class IPCInputMessageReader : public Deserializer<false, false> {

public:
	IPCInputMessageReader(const IPCInputMessage& msg) : Deserializer<false, false>( msg.getUserData(),
											msg.getUserDataLength() ) {
	}

	template<typename T> IPCInputMessageReader& operator>>(T& v) {
		Deserializer::readValue(v);
		return *this;
	}

};

class IPCOutputMessage : public IPCMessage {

public:
	IPCOutputMessage(IPCMessageType type) : m_serializer( getPayload() ) {
		getHeader().m_messageType = type;
		getHeader().m_returnCode = IPCReturnCode::UNDEFINED;
		getHeader().m_requestID = 0;
	}

	IPCOutputMessage(const IPCInputMessage& requestMessage, IPCReturnCode returnCode) : m_serializer( getPayload() ) {
		getHeader().m_messageType = IPCMessageType::ANSWER;
		getHeader().m_requestID = requestMessage.getRequestID();
		getHeader().m_returnCode = returnCode;
	}

	template<typename T> IPCOutputMessage& operator<<(const T& v) {
		m_serializer.writeValue(v);
		return *this;
	}

	ResizeableByteArray& getWritablePayload() {
		return getPayload();
	}

	void assignRequestID() {
		getHeader().m_requestID = nextAvailableRequestID++;
	}

private:
	static IPCRequestID nextAvailableRequestID;
	Serializer<false, false> m_serializer;
};

inline bool IPCInputMessage::isResponseOf(const IPCOutputMessage& outputMessage) const {
	return ( getRequestID() == outputMessage.getRequestID() );
}

}
