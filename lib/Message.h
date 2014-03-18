#pragma once

#include <sys/time.h>

#include "SomeIP-common.h"
#include "SomeIP-Serialization.h"
#include <mutex>

#include "ipc.h"

namespace SomeIP_Lib {

typedef uint16_t ClientIdentifier;
static const ClientIdentifier UNKNOWN_CLIENT = 0xFFFF;

class OutputMessage;

struct InputMessageHeader : public SomeIP::SomeIPHeader {
	ClientIdentifier m_clientIdentifier;
};

/**
 * Input message
 */
class InputMessage {

public:
	InputMessage() :
		m_ipcMessage(NULL), m_bDeleteNeeded(false) {
	}

	InputMessage(const IPCInputMessage& ipcMessage) :
		m_ipcMessage(&ipcMessage), m_bDeleteNeeded(false) {
	}

	bool operator==(const OutputMessage& right) const;

	virtual ~InputMessage() {
		if (m_bDeleteNeeded)
			delete m_ipcMessage;
	}

	void copyFrom(IPCInputMessage& msg) {
		m_ipcMessage = new IPCInputMessage(msg);
		m_bDeleteNeeded = true;
	}

	/**
	 * Returns a string representation of the message
	 */
	std::string toString() const {
		char buffer[10000];
		snprintf( buffer, sizeof(buffer), "MessageID:0x%.4X, MessageType:%s, RequestID:0x%X, payload: %s", getMessageID(),
			  SomeIP::toString( getMessageType() ).c_str(), getHeader().getRequestID(),
			  byteArrayToString( getPayload(), getPayloadLength() ).c_str() );
		return buffer;
	}

	/**
	 * Returns true if the message is a response to the output message passed as parameter
	 */
	bool isAnswerTo(const OutputMessage& msg) const;

	SomeIP::MessageID getMessageID() const {
		return getHeader().getMessageID();
	}

	SomeIP::MessageType getMessageType() const {
		return getHeader().getMessageType();
	}

	void setClientIdentifier(ClientIdentifier identifier) {
		getHeaderPrivate().m_clientIdentifier = identifier;
	}

	ClientIdentifier getClientIdentifier() const {
		return getHeader().m_clientIdentifier;
	}

	const InputMessageHeader& getHeader() const {
		return *reinterpret_cast<const InputMessageHeader*>( m_ipcMessage->getUserData() );
	}

	static size_t getHeaderSize() {
		return sizeof(InputMessageHeader);
	}

	SomeIPInputStream getPayloadInputStream() const {
		SomeIPInputStream stream( getPayload(), getPayloadLength() );
		return stream;
	}

	const void* getPayload() const {
		return m_ipcMessage->getUserData() + sizeof(InputMessageHeader);
	}

	size_t getPayloadLength() const {
		return m_ipcMessage->getUserDataLength() - sizeof(InputMessageHeader);
	}

	const IPCInputMessage& getIPCMessage() const {
		return *m_ipcMessage;
	}

	bool isPingMessage() {
		return (getHeader().getMemberID() == SomeIP::PING_MEMBER_ID);
	}

protected:
	InputMessageHeader& getHeaderPrivate() {
		return *const_cast<InputMessageHeader*>( reinterpret_cast<const InputMessageHeader*>( m_ipcMessage->getUserData() ) );
	}

	const IPCInputMessage* m_ipcMessage;
	bool m_bDeleteNeeded;

};

struct OutputMessageHeader : public SomeIP::SomeIPHeader {
	ClientIdentifier m_clientIdentifier;
};

inline bool byteArraysEqual(const void* p1, size_t length1, const void* p2,
			    size_t length2) {
	if (length1 != length2)
		return false;
	auto pc1 = static_cast<const char*>(p1);
	auto pc2 = static_cast<const char*>(p1);
	for (size_t i = 0; i < length1; i++)
		if (*pc1 != *pc2)
			return false;
	return true;
}

/**
 * Output message
 */
class OutputMessage {

public:
	OutputMessage() :
		m_ipcMessage(IPCMessageType::SEND_MESSAGE) {
		init(0);
	}

	OutputMessage(SomeIP::MessageID messageID) :
		m_ipcMessage(IPCMessageType::SEND_MESSAGE) {
		init(messageID);
	}

	bool operator==(const InputMessage& right) const {
		if ( !( right.getHeader() == getHeader() ) )
			return false;
		if ( !byteArraysEqual( right.getPayload(), right.getPayloadLength(), getPayload(), getPayloadLength() ) )
			return false;
		return true;
	}

	bool operator==(const OutputMessage& right) const {
		if ( !( right.getHeader() == getHeader() ) )
			return false;
		if ( !byteArraysEqual( right.getPayload(), right.getPayloadLength(), getPayload(), getPayloadLength() ) )
			return false;
		return true;
	}

	void init(SomeIP::MessageID messageID) {
		m_headerPosition = getWritablePayload().skip( sizeof(OutputMessageHeader) );
		// Construct header in-place
		new ( &getHeader() )OutputMessageHeader();
		assignUniqueRequestID();
		getHeader().setMessageID(messageID);
	}

	void assignUniqueRequestID() {

		std::lock_guard<std::mutex> lock(s_nextAvailableRequestIDMutex);                // TODO optimize : use an atomic operation

		//		std::atomic_fetch_add(&s_nextAvailableRequestID, 1);
		getHeader().setRequestID(s_nextAvailableRequestID++);
	}

	OutputMessageHeader& getHeader() {
		return *reinterpret_cast<OutputMessageHeader*>(m_ipcMessage.getPayload().getData() + m_headerPosition);
	}

	const OutputMessageHeader& getHeader() const {
		return *reinterpret_cast<const OutputMessageHeader*>(m_ipcMessage.getPayload().getData() + m_headerPosition);
	}

	/**
	 * Returns a string representation of the message
	 */
	std::string toString() const {
		char buffer[1000];
		snprintf( buffer, sizeof(buffer), "MessageID:0x%.4X, MessageType:%s, RequestID:0x%X, Payload : %s",
			  getHeader().getMessageID(), SomeIP::toString( getHeader().getMessageType() ).c_str(),
			  getHeader().getRequestID(), byteArrayToString( getPayload(), getPayloadLength() ).c_str() );
		return buffer;
	}

	SomeIPOutputStream getPayloadOutputStream() {
		SomeIPOutputStream stream( getWritablePayload() );
		return stream;
	}

	void* getPayload() {
		return m_ipcMessage.getUserData() + sizeof(OutputMessageHeader);
	}

	const void* getPayload() const {
		return m_ipcMessage.getUserData() + sizeof(OutputMessageHeader);
	}

	size_t getPayloadLength() const {
		return m_ipcMessage.getUserDataLength() - sizeof(OutputMessageHeader);
	}

	ResizeableByteArray& getWritablePayload() {
		return m_ipcMessage.getWritablePayload();
	}

	ClientIdentifier getClientIdentifier() const {
		return getHeader().m_clientIdentifier;
	}

	void setClientIdentifier(ClientIdentifier clientIdentifier) {
		getHeader().m_clientIdentifier = clientIdentifier;
	}

	const IPCMessage& getIPCMessage() const {
		return m_ipcMessage;
	}

	static uint16_t s_nextAvailableRequestID; // we use only 16 bits since the two MSB are used by the dispatcher to identify the client

	static std::mutex s_nextAvailableRequestIDMutex;

	IPCOutputMessage m_ipcMessage;

	size_t m_headerPosition;

};

typedef InputMessage DispatcherMessage;

inline bool InputMessage::operator==(const OutputMessage& right) const {
	if ( !( getHeader() == right.getHeader() ) )
		return false;
	if ( !byteArraysEqual( right.getPayload(), right.getPayloadLength(), getPayload(), getPayloadLength() ) )
		return false;

	return true;
}

inline OutputMessage createMethodReturn(const InputMessage& inputMessage) {
	OutputMessage methodReturnMessage;
	methodReturnMessage.getHeader().setMessageID( inputMessage.getMessageID() );
	methodReturnMessage.getHeader().setRequestID( inputMessage.getHeader().getRequestID() );
	methodReturnMessage.getHeader().setMessageType(SomeIP::MessageType::RESPONSE);
	methodReturnMessage.setClientIdentifier( inputMessage.getClientIdentifier() );
	return methodReturnMessage;
}

inline bool InputMessage::isAnswerTo(const OutputMessage& msg) const {
	return ( ( getHeader().getRequestID() == msg.getHeader().getRequestID() )
		 && ( (getMessageType() == SomeIP::MessageType::RESPONSE)
		      || (getMessageType() == SomeIP::MessageType::ERROR) ) );
}

InputMessage readMessageFromIPCMessage(const IPCInputMessage& inputMsg);


enum class MessageProcessingResult {
	NotProcessed_OK, Processed_OK, Processed_Error
};

class MessageSource {

public:
	virtual ~MessageSource() {
	}

	virtual SomeIPFunctionReturnCode sendMessage(OutputMessage& msg) = 0;

};

typedef std::function<void (OutputMessage&)> MessageSinkFunction;

class MessageSink {

public:
	virtual ~MessageSink() {
	}

	virtual MessageProcessingResult processMessage(const InputMessage& msg) = 0;

};

class PingSender : public MessageSink {

	LOG_DECLARE_CLASS_CONTEXT("PING", "Ping sender");

public:
	void sendPingMessage(MessageSinkFunction sinkFunction, SomeIP::ServiceID serviceID = SomeIP::DISPATCHER_SERVICE_ID) {
#ifdef ENABLE_PING
		if (!m_pongPending) {
			m_serviceID = serviceID;
			OutputMessage msg( SomeIP::getMessageID(serviceID, SomeIP::PING_MEMBER_ID) );
			msg.getHeader().setMessageType(SomeIP::SomeIPHeader::MessageType::REQUEST);
			gettimeofday(&m_lastPingDate, &tz);
			msg.getPayloadOutputStream();
			sinkFunction(msg);
			m_pongPending = true;
		}
#endif
	}

	void handlePingReply(const InputMessage& msg) {

		struct timeval currentTime;
		struct timeval delay;
		gettimeofday(&currentTime, &tz);

		timeval_subtract(delay, currentTime, m_lastPingDate);

		int delayInMilliseconds = delay.tv_sec * 1000 + delay.tv_usec / 1000;

		if (delayInMilliseconds > maxRTTDelay)
			maxRTTDelay = delayInMilliseconds;

		//		trace_info("Pong received RTTDelay: %i ms, maxRTTDelay:%i ms, ", delayInMilliseconds, maxRTTDelay);
		log_info("Pong received RTTDelay: %i ms", delayInMilliseconds);
		m_pongPending = false;
	}

	int timeval_subtract(struct timeval& result, const struct timeval& x, struct timeval& y) {
		/* Perform the carry for the later subtraction by updating y. */
		if (x.tv_usec < y.tv_usec) {
			int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
			y.tv_usec -= 1000000 * nsec;
			y.tv_sec += nsec;
		}
		if (x.tv_usec - y.tv_usec > 1000000) {
			int nsec = (x.tv_usec - y.tv_usec) / 1000000;
			y.tv_usec += 1000000 * nsec;
			y.tv_sec -= nsec;
		}

		/* Compute the time remaining to wait.
		 tv_usec is certainly positive. */
		result.tv_sec = x.tv_sec - y.tv_sec;
		result.tv_usec = x.tv_usec - y.tv_usec;

		/* Return 1 if result is negative. */
		return x.tv_sec < y.tv_sec;
	}

	MessageProcessingResult processMessage(const InputMessage& msg) {

		if ( (msg.getHeader().getMemberID() == SomeIP::PING_MEMBER_ID) &&
		     (msg.getHeader().getServiceID() == m_serviceID) ) {
			if ( !msg.getHeader().isRequestWithReturn() ) {
				handlePingReply(msg);
				return MessageProcessingResult::Processed_OK;
			}
		}

		return MessageProcessingResult::NotProcessed_OK;
	}

	int maxRTTDelay = 0;
	bool m_pongPending = false;
	struct timeval m_lastPingDate;
	struct timezone tz;
	SomeIP::ServiceID m_serviceID;

};

/**
 * This class offers methods to react to ping requests
 */
class SomeIPEndPoint : public MessageSink {

	LOG_DECLARE_CLASS_CONTEXT("SOEP", "SomeIP endpoint");

public:
	SomeIPEndPoint(MessageSource& source, SomeIP::ServiceID serviceID = SomeIP::DISPATCHER_SERVICE_ID) : m_source(source) {
		m_serviceID = serviceID;
	}

	MessageProcessingResult processMessage(const InputMessage& msg) {

		if ( (msg.getHeader().getMemberID() == SomeIP::PING_MEMBER_ID) &&
		     (msg.getHeader().getServiceID() == m_serviceID) ) {
			if ( msg.getHeader().isRequestWithReturn() ) {
				sendPongMessage(msg);
				return MessageProcessingResult::Processed_OK;
			}
		}

		return MessageProcessingResult::NotProcessed_OK;
	}

protected:
	void sendPongMessage(const InputMessage& msg) {

		log_info("Sending pong");

		// send a pong with the same content
		OutputMessage outputMessage = createMethodReturn(msg);
		outputMessage.getPayloadOutputStream().writeRawData( msg.getPayload(), msg.getPayloadLength() );
		m_source.sendMessage(outputMessage);
	}

private:
	MessageSource& m_source;

private:
	SomeIP::ServiceID m_serviceID;

};


//inline LogData& operator<<(LogData& log, const OutputMessage& v) {
//	log << v.toString();
//	return log;
//}
//
//inline LogData& operator<<(LogData& log, const InputMessage& v) {
//	log << v.toString();
//	return log;
//}

}
