#include "SomeIP-common.h"

#include "MainLoopApplication.h"
#include "CommandLineParser.h"
#include "GlibMainLoopInterfaceImplementation.h"
#include "SomeIP-clientLib.h"

#include <iostream>
#include <string>
#include <boost/tokenizer.hpp>

using namespace SomeIP;
using namespace SomeIP_utils;
using namespace SomeIP_Lib;

LOG_DEFINE_APP_IDS("sctl", "SomeIP ctl tool");
LOG_DECLARE_DEFAULT_CONTEXT(mainContext, "ctl", "Main log context");

typedef std::function<void (const InputMessage&)> MessageReceivedCallbackFunction;

class ControlApp : public MainLoopApplication, public SomeIPClient::ClientConnectionListener {

	LOG_DECLARE_CLASS_CONTEXT("MAIN", "Main");

public:
	ControlApp(SomeIPClient::ClientConnection& connection) :
		MainLoopApplication() {
		connection.setMainLoopInterface(m_glibIntegration);
		connection.connect(*this);
		if ( !connection.isConnected() )
			throw new ConnectionException("Not connected");
	}

	MessageProcessingResult processMessage(const InputMessage& msg) {
		log_info() << "Message received : " << msg;
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

	SomeIPClient::GlibMainLoopInterfaceImplementation m_glibIntegration;

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
	int instanceToRegister = 0;
	MessageID messageToSubscribeTo = NO_MESSAGE_ID;
	bool dumpDaemonState = false;
	bool blockingMode = false;
	int repeatDuration = 0;

	CommandLineParser commandLineParser(
		"Control tool", "message1 message2 ...", SOMEIP_PACKAGE_VERSION,
		"This tools lets you interact with the Some/IP daemon\n"
		"Examples:\n\n"
		"Send a message every 500 ms with serviceID=0x1234, MemberID=543, MessageType=REQUEST, payload=12345678 (hex)\n"
		"someip_ctl 0x1234:543:REQUEST:12345678 -r 500"
		);
	commandLineParser.addOption(serviceToRegister, "service", 's', "A serviceID to register");
	commandLineParser.addOption(instanceToRegister, "instance", 'i', "InstanceID of the service to register");
	commandLineParser.addOption(messageToSubscribeTo, "subscribe", 'n', "A message to subscribe to");
	commandLineParser.addOption(blockingMode, "blocking", 'b', "Use blocking mode");
	commandLineParser.addOption(dumpDaemonState, "dumpDaemonState", 'd', "Dump daemon state");
	commandLineParser.addOption(repeatDuration, "repeatDelay", 'r', "Delay between 2 repetitions");

	if ( commandLineParser.parse(argc, argv) ) {
		commandLineParser.printHelp();
		exit(1);
	}

	SomeIPClient::ClientDaemonConnection connection;
	ControlApp app(connection);

	log_info() << "Control app started. version: " << SOMEIP_PACKAGE_VERSION;

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

		ServiceIDs service(serviceToRegister, instanceToRegister);
		log_info() << "Registering service " << service.toString();
		if (isError(connection.registerService(service)))
			log_error() << "Could not register service: " << service.toString();
	}

	if (messageToSubscribeTo != NO_MESSAGE_ID) {
		SomeIP::MemberIDs memberID(getServiceID(messageToSubscribeTo), 0, getMemberID(messageToSubscribeTo));
		connection.subscribeToNotifications(memberID);
		log_info() << "Subscribe for notifications for messageID:" << memberID.toString();
	}

	if (dumpDaemonState) {
		new GLibTimer( [&]() {
				       std::string s;
				       if ( !isError( connection.getDaemonStateDump(s) ) )
					       std::cout << s << std::endl;
			       }, 1000, app.getMainContext() );

		//		std::cout << connection.getDaemonStateDump();
	}

	auto sendMessagesFunction = [&] () {

		for (int i = 1; i < argc; i++) {

			const char* messageToSend = argv[i];
			auto v = tokenize(messageToSend, ':');

			if (v.size() == 4) {

				ServiceID serviceID = parseIntString(v[0]);
				MemberID memberID = parseIntString(v[1]);
				auto messageType = string2Enum<MessageType> (v[2]);
				auto payloadString = v[3];

				OutputMessage msg;
				msg.getHeader().setServiceID(serviceID);
				msg.getHeader().setMemberID(memberID);
				msg.getHeader().setMessageType(messageType);
				SomeIPOutputStream os = msg.getPayloadOutputStream();
				auto bytes = parseHexPayloadString(payloadString);
				for (size_t i = 0; i < bytes.size(); i++)
					os.writeValue(bytes[i]);

				log_info() << "Sending message : " << msg;

				if (blockingMode) {
					InputMessage answerMessage = connection.sendMessageBlocking(msg);
					log_info() << "Answer : " << answerMessage;
				} else
					connection.sendMessage(msg);

			} else
				std::cerr << "Invalid message format" << std::endl;

		}
	};

	sendMessagesFunction();

	if (repeatDuration != 0) {
		new GLibTimer( [&]() {
				       sendMessagesFunction();
			       }, repeatDuration, app.getMainContext() );
	}

	app.run();

	log_info() << "Shutting down...";

	return 0;

}
