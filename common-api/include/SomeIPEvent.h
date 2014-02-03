#pragma once

#include "CommonAPI-SomeIP.h"

namespace CommonAPI {
namespace SomeIP {

class SomeIPProxy;

template<typename _EventType, typename _ProxyType = SomeIPProxy>
class SomeIPEvent : public _EventType, public MessageSink {
public:
	typedef typename _EventType::ArgumentsTuple ArgumentsTuple;
	typedef typename _EventType::CancellableListener CancellableListener;

	SomeIPEvent(_ProxyType& proxy, MemberID eventName) :
		proxy_(proxy), eventName_(eventName) {
	}

	virtual ~SomeIPEvent() {
		//		assert(false);
		//		if (this->hasListeners())			proxy_.removeSignalMemberHandler(subscription_);
	}

	MessageProcessingResult processMessage(const InputMessage& message) override {
		auto result = unpackArgumentsAndHandleMessage( message, ArgumentsTuple() );
		return (result ==
			SubscriptionStatus::RETAIN ? MessageProcessingResult::Processed_OK : MessageProcessingResult::
			NotProcessed_OK);
	}

protected:
	virtual void onFirstListenerAdded(const CancellableListener&) {
		proxy_.subscribeNotification(eventName_, *this);
	}

	virtual void onLastListenerRemoved(const CancellableListener&) {
		assert(false);
		//	        proxy_.removeSignalMemberHandler(subscription_);
	}

private:
	template<typename ... _Arguments>
	inline SubscriptionStatus unpackArgumentsAndHandleMessage(const InputMessage& message,
								  std::tuple<_Arguments ...> argTuple) {
		return handleMessage( message, std::move(
					      argTuple), typename make_sequence<sizeof ... (_Arguments)>::type() );
	}

	template<typename ... _Arguments, int ... _ArgIndices>
	inline SubscriptionStatus handleMessage(const InputMessage& message, std::tuple<_Arguments ...> argTuple,
						index_sequence<_ArgIndices ...>) {
		SomeIPInputStream inputStream = message.getPayloadInputStream();
		const bool success = SerializableArguments<_Arguments ...>::deserialize(inputStream,
											std::get<_ArgIndices>(argTuple) ...);
		// Continue subscription if deserialization failed
		return success ? this->notifyListeners(std::get<_ArgIndices>(argTuple) ...) : SubscriptionStatus::RETAIN;
	}

	_ProxyType& proxy_;
	MemberID eventName_;
};

}
}
