#pragma once

#include "utilLib/common.h"
#include <string>
#include "stdint.h"

namespace SomeIP {

static const int SERVICE_ID_BITS_COUNT = 16;
typedef uint32_t MessageID;
typedef uint16_t ServiceID;
typedef uint16_t MemberID;
typedef uint16_t InstanceID;
typedef uint16_t EventGroupID;

typedef uint32_t RequestID;
typedef uint8_t ProtocolVersion;
typedef uint8_t InterfaceVersion;

/// MemberID identifying the ping method. TODO : check if Some/IP defines such a thing
static const MemberID PING_MEMBER_ID = 0xFF12;

/// ServiceID identifying the dispatcher itself. TODO : check if Some/IP defines such a thing
static const ServiceID DISPATCHER_SERVICE_ID = 0xFFFE;

enum ReturnCode : uint8_t {
	E_OK
};

inline ServiceID getServiceID(MessageID messageID) {
	return messageID >> SERVICE_ID_BITS_COUNT;
}

inline MemberID getMemberID(MessageID messageID) {
	return messageID & (0xFFFFFFFF >> SERVICE_ID_BITS_COUNT);
}

inline MessageID getMessageID(ServiceID serviceID, MemberID memberID) {
	MessageID messageID = serviceID;
	messageID = (messageID << SERVICE_ID_BITS_COUNT) + memberID;
	return messageID;
}

enum class MessageType
	: uint8_t {
	REQUEST = 0x00, REQUEST_NO_RETURN = 0x01, NOTIFICATION = 0x02, RESPONSE = 0x80, ERROR = 0x81, INVALID = 0xFF, MIN =
		REQUEST, MAX = INVALID
};

struct SomeIPHeader {

public:
	SomeIPHeader& operator=(const SomeIPHeader& right) {
		m_messageID = right.m_messageID;
		m_requestID = right.m_requestID;
		m_interfaceVersion = right.m_interfaceVersion;
		m_messageType = right.m_messageType;
		m_returnCode = right.m_returnCode;
		m_protocolVersion = right.m_protocolVersion;
		return *this;
	}

	SomeIPHeader() {
		m_interfaceVersion = 0;
		m_messageID = m_requestID = m_interfaceVersion = 0;
		m_returnCode = ReturnCode::E_OK;
		m_messageType = MessageType::INVALID;
	}

	SomeIPHeader(MessageID messageID, RequestID requestID, InterfaceVersion interfaceVersion, MessageType messageType,
		     ReturnCode returnCode) {
		m_interfaceVersion = 0;
		m_messageID = messageID;
		m_requestID = requestID;
		m_interfaceVersion = interfaceVersion;
		m_messageType = messageType;
		m_returnCode = returnCode;
	}

	MessageID getMessageID() const {
		return m_messageID;
	}

	MemberID getMemberID() const {
		return SomeIP::getMemberID( getMessageID() );
	}

	MessageType getMessageType() const {
		return m_messageType;
	}

	void setMessageType(MessageType messageType) {
		m_messageType = messageType;
	}

	bool isErrorType() const {
		return getMessageType() == MessageType::ERROR;
	}

	void setMessageID(MessageID messageID) {
		m_messageID = messageID;
	}

	void setServiceID(ServiceID serviceID) {
		m_messageID = SomeIP::getMessageID( serviceID, getMemberID() );
	}

	ServiceID getServiceID() const {
		return SomeIP::getServiceID(m_messageID);
	}

	void setMemberID(MemberID memberID) {
		m_messageID = SomeIP::getMessageID(getServiceID(), memberID);
	}

	bool isReply() const {
		return ( (m_messageType == MessageType::RESPONSE) || (m_messageType == MessageType::ERROR) );
	}

	bool isNotification() const {
		return m_messageType == MessageType::NOTIFICATION;
	}

	bool isRequestWithReturn() const {
		return m_messageType == MessageType::REQUEST;
	}

	void setRequestID(RequestID id) {
		m_requestID = id;
	}

	RequestID getRequestID() const {
		return m_requestID;
	}

	static size_t getLength() {
		return sizeof(SomeIPHeader);
	}

	template<typename Deserializer>
	uint32_t deserialize(Deserializer& deserializer) {
		deserializer >> m_messageID;

		uint32_t length;
		deserializer >> length;

		deserializer >> m_requestID >> m_protocolVersion >> m_interfaceVersion;
		deserializer.readEnum(m_messageType);
		deserializer.readEnum(m_returnCode);

		return length;
	}

	bool operator==(const SomeIPHeader& right) const {
		if (m_messageID != right.m_messageID)
			return false;
		if (m_requestID != right.m_requestID)
			return false;
		if (m_protocolVersion != right.m_protocolVersion)
			return false;
		if (m_interfaceVersion != right.m_interfaceVersion)
			return false;
		if (m_messageType != right.m_messageType)
			return false;
		if (m_returnCode != right.m_returnCode)
			return false;
		return true;
	}

	MessageID m_messageID;
	RequestID m_requestID;
	ProtocolVersion m_protocolVersion = 1;
	InterfaceVersion m_interfaceVersion;
	MessageType m_messageType;
	ReturnCode m_returnCode;

	friend class SomeIPMessage;
};

static constexpr size_t SOMEIP_HEADER_LENGTH_ON_NETWORK = sizeof(SomeIPHeader) + sizeof(uint32_t);

class SomeIPService {
public:
	SomeIPService(ServiceID serviceID) :
		m_serviceID(serviceID) {
	}

	bool matchesRequest(const SomeIPHeader& msg) const {
		return (SomeIP::getServiceID( msg.getMessageID() ) == m_serviceID);
	}

	bool isDuplicate(const ServiceID& serviceID) const {
		return (getServiceID() == serviceID);
	}

	ServiceID getServiceID() const {
		return m_serviceID;
	}

protected:
	ServiceID m_serviceID;

};

class SomeIPServiceDiscoveryHeader : public SomeIPHeader {

public:
	struct Flags {
		bool reboot : 1;
		bool unicast : 1; // means the devices supports unicast. always true according to the spec
		int unused : 6;
	};

	SomeIPServiceDiscoveryHeader() {
		m_flags.reboot = 0;
		m_flags.unicast = 0;
		m_flags.unused = 0;
	}

	bool getRebootFlag() const {
		return m_flags.reboot;
	}

public:
	Flags m_flags;
	uint8_t m_reserved1 = 0;
	uint8_t m_reserved2 = 0;
	uint8_t m_reserved3 = 0;

};

struct SomeIPServiceDiscoveryEntryHeader {

	enum class Type
		: uint8_t {
		FindService = 0x00, OfferService = 0x01, Subscribe = 0x06, SubscribeAck = 0x07, Invalid = 0xFE
	};

	Type m_type = Type::Invalid;

	uint8_t m_indexFirstOptionRun = 0;
	uint8_t m_indexSecondOptionRun = 0;

	uint8_t m_1 = 0;
	uint8_t m_2 = 0;

	ServiceID m_serviceID;
	InstanceID m_instanceID;

	uint8_t m_majorVersion;
	uint32_t m_ttl;
};

inline std::string toString(MessageType messageType) {
	RETURN_ENUM_NAME_IF_EQUAL(messageType, MessageType, REQUEST);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, MessageType, REQUEST_NO_RETURN);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, MessageType, NOTIFICATION);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, MessageType, RESPONSE);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, MessageType, ERROR);
	RETURN_ENUM_NAME_IF_EQUAL(messageType, MessageType, INVALID);
	return "-------";
}

struct SomeIPServiceDiscoveryServiceEntryHeader : public SomeIPServiceDiscoveryEntryHeader {
	uint32_t m_minorVersion;
};

struct SomeIPServiceDiscoveryEventGroupEntryHeader : public SomeIPServiceDiscoveryEntryHeader {
	uint16_t m_reserved16;
	EventGroupID m_eventGroupID;
};

enum class TransportProtocol
	: uint8_t {
	TCP = 0x06, UDP = 0x11
};

}
