#pragma once

#include "gtest/gtest.h"

#include "plog.h"
LOG_DEFINE_APP_IDS("SOTE", "Some/IP test");
LOG_DECLARE_DEFAULT_CONTEXT(blabla, "LOG", "Main context");

using namespace SomeIP_Lib;
using namespace pelagicore;

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
		log_info("message received :") << msg;
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
