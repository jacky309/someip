//#include "CommonAPI-SomeIP.h"
#include "SomeIP-Serialization.h"
#include "SomeIP-clientLib.h"

#include "GlibMainLoopInterfaceImplementation.h"

#include "test-common.h"

#include "MainLoopApplication.h"
#include "Message.h"

class MyClass {

public:
	MyClass(int i, const char* s) {
		m_int = i;
		m_string = s;
	}

	bool operator==(const MyClass& right) const {
		if ( !(right.m_string == m_string) )
			return false;
		if ( !(right.m_int == m_int) )
			return false;
		return true;
	}

	std::string m_string;
	int m_int;
};

inline SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const MyClass& v) {
	stream << v.m_string;
	stream << v.m_int;
	return stream;
}

inline SomeIPInputStream& operator>>(SomeIPInputStream& stream, MyClass& v) {
	stream >> v.m_string;
	stream >> v.m_int;
	return stream;
}

void sendMessageWithExpectedAnswer(     //SomeIPClient::ClientConnection& connection,
	OutputMessage& outputMsg, OutputMessage& expectedMsg) {

	using namespace SomeIPClient;

	ClientDaemonConnection connection;

	CommonAPI::Version v;

	log_info() << "Sending message : " << outputMsg;
	log_info() << "Expected message : " << expectedMsg;

	TestSink sink(
		[&](const InputMessage &msg) {
			log_info() << "Received message : " << msg;
			EXPECT_TRUE( msg.isAnswerTo(expectedMsg) );
			EXPECT_TRUE( byteArraysEqual( msg.getPayload(), msg.getPayloadLength(),
						      expectedMsg.getPayload(), expectedMsg.getPayloadLength() ) );
		});

	GlibMainLoopInterfaceImplementation glibIntegration;
	connection.setMainLoopInterface(glibIntegration);

	connection.connect(sink);

	EXPECT_EQ(connection.isConnected(), true);

	MainLoopApplication app;

	connection.sendMessage(outputMsg);
	app.run(1000);

	EXPECT_EQ(sink.getReceivedMessageCount(), 1);

}

OutputMessage createOutputMessage(SomeIP::ServiceID service, SomeIP::MessageType messageType,
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


TEST_F(SomeIPTest, Serializationcustom) {

	ByteArray byteArray;
	SomeIPOutputStream stream(byteArray);

	const MyClass referenceValue(543, "fdsgds");
	stream << referenceValue;

	log_debug("Serialization : ") << byteArray;

	SomeIPInputStream inputStream( byteArray.getData(), byteArray.size() );

	MyClass readValue(0, "");
	inputStream >> readValue;
	EXPECT_EQ(readValue, referenceValue);
}

TEST_F(SomeIPTest, SerializationInt) {

	ByteArray byteArray;
	SomeIPOutputStream stream(byteArray);

	const int referenceInt = 0x12345678;
	stream << referenceInt;

	log_debug("Serialization : ") << byteArray;

	SomeIPInputStream inputStream( byteArray.getData(), byteArray.size() );

	int readInt;
	inputStream >> readInt;
	EXPECT_EQ(readInt, referenceInt);
}

TEST_F(SomeIPTest, SerializationString) {

	ByteArray byteArray;
	SomeIPOutputStream stream(byteArray);

	const char* referenceString = "This is a string";
	stream << referenceString;

	log_debug("Serialization : ") << byteArray;

	SomeIPInputStream inputStream( byteArray.getData(), byteArray.size() );

	std::string readString;
	inputStream >> readString;
	EXPECT_EQ(readString, referenceString);
}

TEST_F(SomeIPTest, SerializationSimple) {

	ByteArray byteArray;
	SomeIPOutputStream stream(byteArray);

	const char* referenceString = "This is a string";
	stream << referenceString;

	const int referenceInt = 0x12435436;
	stream << referenceInt;

	const bool referenceBool = 1;
	stream << referenceBool;

	SomeIPInputStream inputStream( byteArray.getData(), byteArray.size() );

	std::string readString;
	inputStream >> readString;
	EXPECT_EQ(readString, referenceString);

	int readInt;
	inputStream >> readInt;
	EXPECT_EQ(readInt, referenceInt);

	bool readBool;
	inputStream >> readBool;
	EXPECT_EQ(readBool, referenceBool);
}

TEST_F(SomeIPTest, SerializationComplex) {

	ByteArray byteArray;
	SomeIPOutputStream stream(byteArray);

	CommonAPI::Variant<std::string, uint16_t> referenceVariant1;
	uint16_t anInt = 435;
	referenceVariant1 = anInt;
	stream << referenceVariant1;

	CommonAPI::Variant<uint32_t, std::string, float> referenceVariant2;
	std::string aString = "AString";
	referenceVariant2 = aString;
	stream << referenceVariant2;

	std::unordered_map<uint16_t, std::string> referenceMap;
	referenceMap[43] = "ABCEDF";
	referenceMap[58] = "EFGHI";
	stream << referenceMap;

	const char* referenceString = "This is a string";
	stream << referenceString;

	const int referenceInt = 0x12435436;
	stream << referenceInt;

	const bool referenceBool = 1;
	stream << referenceBool;

	SomeIPInputStream inputStream( byteArray.getData(), byteArray.size() );

	CommonAPI::Variant<std::string, uint16_t> readVariant1;
	inputStream >> readVariant1;
	EXPECT_EQ(referenceVariant1, readVariant1);

	CommonAPI::Variant<uint32_t, std::string, float> readVariant2;
	inputStream >> readVariant2;
	EXPECT_EQ(referenceVariant2, readVariant2);

	std::unordered_map<uint16_t, std::string> readMap;
	inputStream >> readMap;
	EXPECT_EQ(readMap, referenceMap);

	std::string readString;
	inputStream >> readString;
	EXPECT_EQ(readString, referenceString);

	int readInt;
	inputStream >> readInt;
	EXPECT_EQ(readInt, referenceInt);

	bool readBool;
	inputStream >> readBool;
	EXPECT_EQ(readBool, referenceBool);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	auto ret = RUN_ALL_TESTS();
	return ret;
}
