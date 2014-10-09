#pragma once

#include "gtest/gtest.h"

#include "ivi-logging.h"

using namespace SomeIP_Lib;

LOG_DECLARE_DEFAULT_CONTEXT(blabla, "LOG", "Main context");

class TestSink : public SomeIPClient::ClientConnectionListener {
public:
	typedef std::function<void (const InputMessage& msg)> CallBackFunction;

	TestSink(CallBackFunction function) :
		m_callBackFunction(function) {
	}

	void setCallbackFunction(CallBackFunction function) {
		m_callBackFunction = function;
	}

	MessageProcessingResult processMessage(const InputMessage& msg) override {
		log_info() << "message received :" << msg;
		m_callBackFunction(msg);
		m_messageCounter++;
		return MessageProcessingResult::Processed_OK;
	}

	void onDisconnected() {
		//		EXPECT_TRUE(false);
	}

	size_t getReceivedMessageCount() {
		return m_messageCounter;
	}

private:
	CallBackFunction m_callBackFunction;
	size_t m_messageCounter = 0;

};


class SomeIPTest : public::testing::Test {

protected:
	// You can remove any or all of the following functions if its body
	// is empty.

	SomeIPTest() {
		// You can do set-up work for each test here.
	}

	virtual ~SomeIPTest() {
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	//	virtual void SetUp() {
	//		// Code here will be called immediately after the constructor (right before each test).
	//		log_error();
	//	}

	//	virtual void TearDown() {
	//		// Code here will be called immediately after each test (right before the destructor).
	//		log_error();
	//	}

};


inline OutputMessage createTestOutputMessage(SomeIP::ServiceIDs service, SomeIP::MessageType messageType,
		size_t contentLength) {
	OutputMessage outputMsg;
	OutputMessageHeader& header = outputMsg.getHeader();
	header.setServiceID(service.serviceID);
	outputMsg.setInstanceID(service.instanceID);
	header.setMessageType(messageType);
	SomeIPOutputStream os = outputMsg.getPayloadOutputStream();

	for (size_t i = 0; i < contentLength; i++) {
		int8_t v = i;
		os << v;
	}

	return outputMsg;
}
