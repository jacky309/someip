#pragma once

#include <CommonAPI/Stub.h>
#include "Message.h"
#include "SomeIPConnection.h"

namespace CommonAPI {
namespace SomeIP {

class SomeIPStubAdapter : virtual public CommonAPI::StubAdapter, public MessageSink, private MessageSource {
public:
	SomeIPStubAdapter(const std::string& commonApiAddress, ServiceID serviceID, SomeIPConnection& connection) :
		connection_(connection), m_someIPEndPoint(*this, serviceID), serviceID_(serviceID) {
	}

	virtual ~SomeIPStubAdapter() {
		deinit();
	}

	std::string toString() const {
		return "SomeIPStubAdapter";
	}

	SomeIPReturnCode sendMessage(OutputMessage& msg);

	virtual void init();
	virtual void deinit();

	const std::string getAddress() const override {
		assert(false);
		return "";
	}

	const std::string& getDomain() const override {
		return commonApiDomain_;
	}

	const std::string& getServiceId() const override {
		return commonApiServiceId_;
	}

	const std::string& getInstanceId() const override {
		assert(false);
		return m_instanceID;
	}

	ServiceID getServiceID() const {
		return serviceID_;
	}

	inline SomeIPConnection& getConnection() const {
		return connection_;
	}

	MessageProcessingResult processMessage(const InputMessage& message) {
		return m_someIPEndPoint.processMessage(message);
	}

private:
	const std::string commonApiDomain_;
	const std::string commonApiServiceId_;
	const std::string commonApiParticipantId_;
	std::string m_instanceID;

	SomeIPConnection& connection_;
	SomeIPEndPoint m_someIPEndPoint;
	ServiceID serviceID_;

	bool m_isRegistered = false;

	static const std::string domain_;
};

}
}
