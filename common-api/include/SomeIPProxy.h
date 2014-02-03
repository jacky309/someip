#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

//#include "CommonAPI/CommonAPI.h"
#include "Attribute.h"
#include "CommonAPI-SomeIP.h"
#include "SerializableArguments.h"

namespace CommonAPI {
namespace SomeIP {


class SomeIPProxy : public virtual CommonAPI::Proxy, private ProxyStatusEvent,
	private SomeIPClient::ServiceAvailabilityListener, public MessageSink {
public:
	SomeIPProxy(SomeIPConnection& connection, const std::string& commonApiAddress,
		    ServiceID serviceID) : ServiceAvailabilityListener(serviceID), connection_(connection),
		commonApiAddress_(commonApiAddress), interfaceVersionAttribute_(*this,
										GET_INTERFACE_VERSION_MEMBER_ID),
		m_serviceID(serviceID) {
	}

	SomeIPProxy(const SomeIPProxy&) = delete;

	virtual ~SomeIPProxy() {
	}

	void onListenerAdded(const CancellableListener& listener) override {
		m_availabilityListeners.push_back(listener);
		if ( isAvailable() )
			listener(AvailabilityStatus::AVAILABLE);
	}

	// Events sent during unsubscribe()
	void onListenerRemoved(const CancellableListener& listener) override {
		// TODO :: implement
		assert(false);
	}

	bool isAvailable() const override {
		return availabilityStatus_ == AvailabilityStatus::AVAILABLE;
	}

	ProxyStatusEvent& getProxyStatusEvent() override {
		return *this;
	}

	InterfaceVersionAttribute& getInterfaceVersionAttribute() override {
		return interfaceVersionAttribute_;
	}

	virtual bool isAvailableBlocking() const {
		return isAvailable();
		//		assert(false);
	}

	std::string getAddress() const override {
		return commonApiAddress_;
	}

	const std::string& getDomain() const override {
		assert(false);
		return commonApiAddress_;
	}

	const std::string& getServiceId() const override {
		assert(false);
		return commonApiAddress_;
	}

	const std::string& getInstanceId() const override {
		assert(false);
		return commonApiAddress_;
	}

	ServiceID getServiceID() const {
		return m_serviceID;
	}

	SomeIPConnection& getConnection() {
		return connection_;
	}

	OutputMessage createMethodCall(MemberID memberID) const {
		OutputMessage msg( SomeIP::getMessageID(getServiceID(), memberID) );
		msg.getHeader().setMessageType(SomeIP::MessageType::REQUEST);
		return msg;
	}

	void removeSignalMemberHandler() {
		assert(false);
	}

	void init() {
		connection_.registerProxy(*this);
	}

	void sendPingMessage(SomeIP::ServiceID serviceID) {

		auto pingSender = new PingSender();
		pingSender->sendPingMessage([&] (OutputMessage & msg) {
						    getConnection().sendWithResponseHandler(msg, *pingSender);
					    }, serviceID);
	}

	void subscribeNotification(MemberID memberID, MessageSink& handler) {
		getConnection().subscribeNotification( SomeIP::getMessageID(getServiceID(), memberID) );
		m_messageHandlers[memberID] = &handler;
	}

	void onServiceAvailable() override {
		availabilityStatus_ = AvailabilityStatus::AVAILABLE;
		for (auto& listener : m_availabilityListeners) {
			listener(availabilityStatus_);
		}
	}

	void onServiceUnavailable() override {
		availabilityStatus_ = AvailabilityStatus::NOT_AVAILABLE;
		for (auto& listener : m_availabilityListeners) {
			listener(availabilityStatus_);
		}
	}

	MessageProcessingResult processMessage(const InputMessage& message) override;

	SomeIPConnection& connection_;

	const std::string commonApiServiceId_;
	const std::string commonApiParticipantId_;
	const std::string commonApiAddress_;

	SomeIPReadonlyAttribute<InterfaceVersionAttribute> interfaceVersionAttribute_;

	std::unordered_map<MemberID, MessageSink*> m_messageHandlers;

	std::vector<CancellableListener> m_availabilityListeners;

	ServiceID m_serviceID;
	AvailabilityStatus availabilityStatus_ = AvailabilityStatus::NOT_AVAILABLE;

	friend class SomeIPConnection;
};


}

}
