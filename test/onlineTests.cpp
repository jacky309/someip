#include "CommonAPI-SomeIP.h"
#include "SomeIP-Serialization.h"
#include "SomeIP-clientLib.h"

#include "GlibClientConnection.h"

#include "test-common.h"

#include "MainLoopApplication.h"
#include "Message.h"

static const SomeIP::ServiceID TEST_SERVICE_ID = 0x543F;

static const int TIMEOUT = 200;
static const size_t MESSAGE_SIZE = 65536;

void sendMessageWithExpectedAnswer(OutputMessage& outputMsg, OutputMessage& expectedMsg) {

	using namespace SomeIPClient;

	ClientDaemonConnection connection;

	log_info("Sending message : ") << outputMsg;
	log_info("Expected message : ") << expectedMsg;

	TestSink sink(
		[&](const InputMessage &msg) {
			log_info("Received message : ") << msg;
			log_info("Expected message : ") << expectedMsg;
			EXPECT_TRUE( msg.isAnswerTo(expectedMsg) );
			EXPECT_TRUE(msg == expectedMsg);
			EXPECT_TRUE( byteArraysEqual( msg.getPayload(), msg.getPayloadLength(),
						      expectedMsg.getPayload(), expectedMsg.getPayloadLength() ) );
		});

	connection.connect(sink);

	EXPECT_EQ(connection.isConnected(), true);

	GLibIntegration glibIntegration(connection);
	glibIntegration.setup();
	MainLoopApplication app;

	connection.sendMessage(outputMsg);
	app.run(TIMEOUT);

	EXPECT_EQ(sink.getReceivedMessageCount(), 1);

}

OutputMessage createTestOutputMessage(SomeIP::ServiceID service, SomeIP::MessageType messageType,
				      size_t contentLength) {
	OutputMessage outputMsg;
	OutputMessageHeader& header = outputMsg.getHeader();
	header.setServiceID(service);
	header.setMessageType(messageType);
	SomeIPOutputStream os = outputMsg.getPayloadOutputStream();

	for (size_t i = 0; i < contentLength; i++) {
		int8_t v = i;
		os << v;
	}

	return outputMsg;
}

/**
 * Set up two connections to the dispatcher successively, to check whether the first connection is properly uninitialized so that the
 * second one can be properly established.
 */
TEST_F(SomeIPTest, ConnectionCleanup) {

	using namespace SomeIPClient;

	{
		ClientDaemonConnection connection;

	TestSink sink([&](const InputMessage &msg) {
		      });

	connection.connect(sink);
	log_error() << "TTTTTTT" << connection.isConnected();
	EXPECT_TRUE(connection.isConnected());
	GLibIntegration glibIntegration(connection);
	glibIntegration.setup();
	EXPECT_FALSE(isError(connection.registerService(TEST_SERVICE_ID)));
	}

	{
		ClientDaemonConnection connection;

		OutputMessage outputMsg = createTestOutputMessage(TEST_SERVICE_ID, SomeIP::MessageType::REQUEST,
								  MESSAGE_SIZE);

		TestSink sink([&](const InputMessage &msg) {
				      log_info() << msg;
				      EXPECT_EQ(msg, outputMsg);
			      });

		connection.connect(sink);
		GLibIntegration glibIntegration(connection);
		glibIntegration.setup();

		connection.registerService(TEST_SERVICE_ID);

		connection.sendMessage(outputMsg);

		MainLoopApplication app;
		app.run(TIMEOUT);

		// We expect one answer to be received
		EXPECT_EQ(sink.getReceivedMessageCount(), 1);
	}

}

struct TestConnection {

	TestConnection() :
		sink([&](const InputMessage &msg) {
		     }), glibIntegration(connection) {
	}

	void init() {

		OutputMessage testOutputMsg = createTestOutputMessage(TEST_SERVICE_ID, SomeIP::MessageType::REQUEST,
								      MESSAGE_SIZE);
		connection.connect(sink);

		glibIntegration.setup();

		connection.registerService(TEST_SERVICE_ID);

		OutputMessage expectedMsg = testOutputMsg;
		expectedMsg.getHeader().setMessageType(SomeIP::MessageType::RESPONSE);

		connection.sendMessage(testOutputMsg);

	}

	SomeIPClient::ClientDaemonConnection connection;
	TestSink sink;
	SomeIPClient::GLibIntegration glibIntegration;

};


/**
 * Stress the dispatcher with plenty of connection requests.
 */
TEST_F(SomeIPTest, ClientConnectionStressTest) {

	using namespace SomeIPClient;

	std::vector<std::unique_ptr<TestConnection> > v1;

	for (int i = 0; i < 128; i++) {
	auto p = std::unique_ptr<TestConnection>( new TestConnection() );
	p->init();
	v1.push_back( std::move(p) );

	std::vector<std::unique_ptr<TestConnection> > v2;
	for (int j = 0; j < i % 10; j++) {
		auto p = std::unique_ptr<TestConnection>( new TestConnection() );
		p->init();
		v2.push_back( std::move(p) );
	}

	}

}


/**
 * Send a message to ourself, using two distinct connections.
 */
TEST_F(SomeIPTest, SendSelf) {

	using namespace SomeIPClient;

	ClientDaemonConnection connection;

	OutputMessage testOutputMsg = createTestOutputMessage(TEST_SERVICE_ID, SomeIP::MessageType::REQUEST,
							      MESSAGE_SIZE);

	TestSink sink(
		[&](const InputMessage &msg) {
			EXPECT_TRUE(msg.getHeader().getServiceID() == TEST_SERVICE_ID);
			EXPECT_TRUE(msg == testOutputMsg);
			OutputMessage returnMessage = createMethodReturn(msg);
			returnMessage.getPayloadOutputStream().writeRawData( msg.getPayload(), msg.getPayloadLength() );
			connection.sendMessage(returnMessage);
		});

	connection.connect(sink);
	GLibIntegration glibIntegration(connection);
	glibIntegration.setup();

	connection.registerService(TEST_SERVICE_ID);

	OutputMessage expectedMsg = testOutputMsg;
	expectedMsg.getHeader().setMessageType(SomeIP::MessageType::RESPONSE);

	sendMessageWithExpectedAnswer(testOutputMsg, expectedMsg);

	EXPECT_EQ(sink.getReceivedMessageCount(), 1);

}


TEST_F(SomeIPTest, SendToUnregisteredService) {

	OutputMessage outputMsg = createTestOutputMessage(TEST_SERVICE_ID, SomeIP::MessageType::REQUEST,
							  MESSAGE_SIZE);

	OutputMessage expectedMsg;
	expectedMsg.getHeader() = outputMsg.getHeader();
	expectedMsg.getHeader().setMessageType(SomeIP::MessageType::ERROR);

	sendMessageWithExpectedAnswer(outputMsg, expectedMsg);

}


int main(int argc, char** argv) {
	MainLoopApplication app; // to get rid of the DLT threads, by forcing the DLT main loop mode
	::testing::InitGoogleTest(&argc, argv);
	auto ret = RUN_ALL_TESTS();
	return ret;
}
