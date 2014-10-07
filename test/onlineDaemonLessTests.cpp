#include "CommonAPI-SomeIP.h"
#include "SomeIP-Serialization.h"
#include "SomeIP-standaloneClientLib.h"

#include "GlibMainLoopInterfaceImplementation.h"

#include "test-common.h"

#include "MainLoopApplication.h"
#include "Message.h"

static const SomeIP::ServiceID TEST_SERVICE_ID = 0x543F;

//static const int TIMEOUT = 200;
//static const size_t MESSAGE_SIZE = 65536;
//
//struct TestConnection {
//
//	TestConnection() :
//		sink([&](const InputMessage &msg) {
//		     }) {
//	}
//
//	void init() {
//
//		connection.setMainLoopInterface(glibIntegration);
//		connection.connect(sink);
//
//		connection.registerService(TEST_SERVICE_ID);
//
//		OutputMessage expectedMsg = testOutputMsg;
//		expectedMsg.getHeader().setMessageType(SomeIP::MessageType::RESPONSE);
//
//		connection.sendMessage(testOutputMsg);
//
//	}
//
//	SomeIPClient::ClientDaemonConnection connection;
//	TestSink sink;
//	SomeIPClient::GlibMainLoopInterfaceImplementation glibIntegration;
//
//};


/**
 * Test that a service registered using one connection is also visible with another connection
 */
TEST_F(SomeIPTest, RegisterServiceTest) {

	using namespace SomeIPClient;

	GlibMainLoopInterfaceImplementation glibIntegration;
	TestSink sink(
		[&](const InputMessage &msg) {
		});

	DaemonLessClient connection1(glibIntegration);
	connection1.setMainLoopInterface(glibIntegration);
	connection1.connect(sink);
	connection1.registerService(TEST_SERVICE_ID);

	DaemonLessClient connection2(glibIntegration);
	connection2.setMainLoopInterface(glibIntegration);
	connection2.connect(sink);

	MainLoopApplication app;

	app.run(2000);

	EXPECT_TRUE(connection2.getServiceRegistry().isServiceRegistered(TEST_SERVICE_ID));


	connection1.unregisterService(TEST_SERVICE_ID);

	app.run(2000);

	EXPECT_FALSE(connection2.getServiceRegistry().isServiceRegistered(TEST_SERVICE_ID));

}


int main(int argc, char** argv) {
	MainLoopApplication app; // to get rid of the DLT threads, by forcing the DLT main loop mode
	::testing::InitGoogleTest(&argc, argv);
	auto ret = RUN_ALL_TESTS();
	return ret;
}
