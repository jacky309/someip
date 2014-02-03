#pragma once

#include "ProxyHelper.h"
#include "SomeIPEvent.h"

#include <assert.h>

namespace CommonAPI {
namespace SomeIP {

class SomeIPProxy;

template<typename _AttributeType, typename _ProxyType = SomeIPProxy>
class SomeIPReadonlyAttribute : public _AttributeType {
public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

	SomeIPReadonlyAttribute(_ProxyType& proxy, MemberID getMethodName) :
		proxy_(proxy), getMethodName_(getMethodName) {
	}

	void getValue(CallStatus& callStatus, ValueType& value) const {
		SomeIPProxyHelper<SerializableArguments<>, SerializableArguments<ValueType> >::callMethodWithReply(
			proxy_,
			getMethodName_,
			callStatus, value);
	}

	std::future<CallStatus> getValueAsync(AttributeAsyncCallback attributeAsyncCallback) {
		return SomeIPProxyHelper<SerializableArguments<>, SerializableArguments<ValueType> >::callMethodAsync(
			       proxy_,
			       getMethodName_,
			       std::move(attributeAsyncCallback) );
	}

protected:
	_ProxyType& proxy_;
	MemberID getMethodName_;
};

template<typename _AttributeType, typename _ProxyType = SomeIPProxy>
class SomeIPAttribute : public SomeIPReadonlyAttribute<_AttributeType> {
public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

	SomeIPAttribute(_ProxyType& proxy, MemberID setMethodID, MemberID getMethodName) :
		SomeIPReadonlyAttribute<_AttributeType>(proxy, getMethodName), setMethodID_(setMethodID) {
	}

	void setValue(const ValueType& requestValue, CallStatus& callStatus, ValueType& responseValue) {
		SomeIPProxyHelper<SerializableArguments<ValueType>, SerializableArguments<ValueType> >::callMethodWithReply(
			this->proxy_,
			setMethodID_,
			requestValue, callStatus, responseValue);
	}

	std::future<CallStatus> setValueAsync(const ValueType& requestValue, AttributeAsyncCallback attributeAsyncCallback) {
		return SomeIPProxyHelper<SerializableArguments<ValueType>, SerializableArguments<ValueType> >::callMethodAsync(
			       this->proxy_, setMethodID_, requestValue, attributeAsyncCallback);
	}

protected:
	const MemberID setMethodID_;
};

template<typename _AttributeType, typename _ProxyType = SomeIPProxy>
class SomeIPObservableAttribute : public _AttributeType {
public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;
	typedef typename _AttributeType::ChangedEvent ChangedEvent;

	template<typename ... _AttributeTypeArguments>
	SomeIPObservableAttribute(_ProxyType& proxy, MemberID changeNotificationMemberID,
				  _AttributeTypeArguments ... arguments) : _AttributeType(proxy,
											  arguments
											  ...),
		changedEvent_(proxy, changeNotificationMemberID) {
	}

	ChangedEvent& getChangedEvent() {
		return changedEvent_;
	}

protected:
	SomeIPEvent<ChangedEvent> changedEvent_;
};

}
}
