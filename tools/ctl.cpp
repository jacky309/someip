#include "SomeIP-common.h"

LOG_DEFINE_APP_IDS("sctl", "SomeIP ctl tool");
LOG_DECLARE_DEFAULT_CONTEXT(mainContext, "ctl", "Main log context");

#include "MainLoopApplication.h"
#include "CommandLineParser.h"
#include "SomeIP-clientLib.h"
#include "GlibIO.h"
#include <iostream>
#include <string>
#include <boost/tokenizer.hpp>

using namespace SomeIP;
using namespace SomeIP_utils;
using namespace SomeIP_Lib;

typedef std::function<void (const InputMessage&)> MessageReceivedCallbackFunction;

class ControlApp : public MainLoopApplication, public SomeIPClient::ClientConnectionListener {
public:
	ControlApp(SomeIPClient::ClientConnection& connection) :
		MainLoopApplication(), m_glibIntegration(connection, NULL) {
		connection.connect(*this);
		m_glibIntegration.setup();
	}

	MessageProcessingResult processMessage(const InputMessage& msg) {
		log_info( "Message received : %s", msg.toString().c_str() );
		for (auto& listener : m_messageListeners) {
			listener(msg);
		}
		return MessageProcessingResult::Processed_OK;
	}

	void onDisconnected() override {
		ControlApp::exit();
	}

	void addMessageReceivedListener(MessageReceivedCallbackFunction listener) {
		m_messageListeners.push_back(listener);
	}

	SomeIPClient::GLibIntegration m_glibIntegration;

	std::vector<MessageReceivedCallbackFunction> m_messageListeners;
};

static const ServiceID NO_SERVICE_ID = 0xFFFF;
static const MessageID NO_MESSAGE_ID = 0xFFFFFFFF;

std::vector<std::string> tokenize(const char* str, char delimiter) {

	std::stringstream ss(str);
	std::string s;
	std::vector<std::string> v;

	while ( getline(ss, s, delimiter) ) {
		v.push_back(s);
	}

	return v;
}

/**
 * Parse a decimal or hexadecimal integer string representation, with auto-detection of the radix
 */
int parseIntString(const std::string& str) {

	std::stringstream stream(str);

	if (str.size() >= 2)
		if ( (str.substr(0, 2) == "0x") || (str.substr(0, 2) == "0X") )
			stream >> std::hex;

	log_debug()
	<< str.substr(2);
	int a;
	stream >> a;
	return a;
}

ByteArray parseHexPayloadString(const std::string& str) {

	ByteArray array;

	assert(str.size() % 2 == 0);

	for (size_t i = 0; i < str.size(); i += 2) {
		char digits[3] = {str[i], str[i + 1], 0};
		unsigned char c = g_ascii_strtoull(digits, NULL, 16);
		array.append(c);
	}

	return array;
}

int main(int argc, const char** argv) {

	int serviceToRegister = NO_SERVICE_ID;
	MessageID messageToSubscribeTo = NO_MESSAGE_ID;
	bool dumpDaemonState = false;
	bool blockingMode = true;

	CommandLineParser commandLineParser("Control tool", "message1 message2 ...", SOMEIP_PACKAGE_VERSION,
					    "This tools lets you interact with the Some/IP daemon");
	commandLineParser.addArgument(serviceToRegister, "service", 's', "A service to register");
	commandLineParser.addArgument(messageToSubscribeTo, "subscribe", 'n', "A message to subscribe to");
	//	commandLineParser.addArgument(messageToSend, "message", 'm', "A message to be sent.");
	commandLineParser.addArgument(blockingMode, "blocking", 'b', "Use blocking mode");
	commandLineParser.addArgument(dumpDaemonState, "dumpDaemonState", 'd', "Dump daemon state");

	if ( commandLineParser.parse(argc, argv) ) {
		commandLineParser.printHelp();
		exit(1);
	}

	SomeIPClient::ClientConnection connection;
	ControlApp app(connection);

	log_info("Control app started. version: ") << SOMEIP_PACKAGE_VERSION;

	if (serviceToRegister != NO_SERVICE_ID) {
		app.addMessageReceivedListener([&] (const InputMessage &inputMsg) {
						       if ( inputMsg.getHeader().isRequestWithReturn() ) {
							       OutputMessage outputMessage = createMethodReturn(inputMsg);
							       SomeIPOutputStream outputStream( outputMessage.getWritablePayload() );
							       outputStream.writeRawData( inputMsg.getPayload(),
											  inputMsg.getPayloadLength() );
							       outputStream.writeRawData( inputMsg.getPayload(),
											  inputMsg.getPayloadLength() );
							       connection.sendMessage(outputMessage);
						       }
					       });

		connection.registerService(serviceToRegister);
		log_info("Registering service serviceID:0x%X", serviceToRegister);
	}

	if (messageToSubscribeTo != NO_MESSAGE_ID) {
		connection.subscribeToNotifications(messageToSubscribeTo);
		log_info("Subscribe for notifications for messageID:0x%X", messageToSubscribeTo);
	}

	if (dumpDaemonState) {
		new GLibTimer([&]() {
				      std::cout << connection.getDaemonStateDump();
			      }, 1000);

		std::cout << connection.getDaemonStateDump();
	}

	for (int i = 1; i < argc; i++) {

		const char* messageToSend = argv[i];
		auto v = tokenize(messageToSend, ':');

		//		log_debug() << v;

		if (v.size() == 4) {

			ServiceID serviceID = parseIntString(v[0]);
			MemberID memberID = parseIntString(v[1]);
			auto messageType = string2Enum<MessageType> (v[2]);
			auto payloadString = v[3];

			OutputMessage msg;
			msg.getHeader().setServiceID(serviceID);
			msg.getHeader().setMemberID(memberID);
			msg.getHeader().setMessageType(messageType);
			//			msg.getHeader().setRequestID(0x4321);
			SomeIPOutputStream os = msg.getPayloadOutputStream();
			auto bytes = parseHexPayloadString(payloadString);
			for (size_t i = 0; i < bytes.size(); i++)
				os.writeValue(bytes[i]);

			log_info( "Sending message : %s", msg.toString().c_str() );

			if (blockingMode) {
				InputMessage answerMessage = connection.sendMessageBlocking(msg);
				log_info( "Answer : %s", answerMessage.toString().c_str() );
			} else
				connection.sendMessage(msg);

		} else
			std::cerr << "Invalid message format";

	}

	app.run();

	log_info("Shutting down...");

	return 0;

}
