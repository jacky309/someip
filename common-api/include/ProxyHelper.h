#pragma once

#include "SerializableArguments.h"
#include "SomeIPProxy.h"
#include "SomeIPStubAdapter.h"
#include "SomeIPHelper.h"
#include "CommonAPI-SomeIP.h"
#include "SomeIPRuntime.h"

#include <functional>
#include <future>
#include <memory>
#include <string>

namespace CommonAPI {
namespace SomeIP {

class SomeIPProxy;

template<class, class>
struct SomeIPProxyHelper;

template<template<class ...> class _In, class ... _InArgs,
	 template<class ...> class _Out, class ... _OutArgs>
struct SomeIPProxyHelper<_In<_InArgs ...>, _Out<_OutArgs ...> > {
	template<typename _ProxyType = SomeIPProxy>
	static void callMethod(const _ProxyType& proxy,
			       MemberID memberID,
			       const _InArgs& ... inArgs,
			       CommonAPI::CallStatus& callStatus) {
		if ( proxy.isAvailableBlocking() ) {

			OutputMessage message = proxy.createMethodCall(memberID);

			if (sizeof ... (_InArgs) > 0) {
				SomeIPOutputStream outputStream = message.getPayloadOutputStream();
				const bool success = SerializableArguments<_InArgs ...>::serialize(outputStream, inArgs ...);
				if (!success) {
					callStatus = CallStatus::OUT_OF_MEMORY;
					return;
				}
			}

			const bool messageSent = proxy.getConnection()->send(message);
			callStatus = messageSent ? CallStatus::SUCCESS : CallStatus::OUT_OF_MEMORY;
		} else {
			log_warn( "Proxy is not available: ServiceID:%i", proxy.getServiceID() );
			callStatus = CallStatus::NOT_AVAILABLE;
		}
	}

	template<typename _ProxyType = SomeIPProxy>
	static void callMethodWithReply(
		_ProxyType& proxy,
		MemberID memberID,
		const _InArgs& ... inArgs,
		CommonAPI::CallStatus& callStatus,
		_OutArgs& ... outArgs) {

		if ( proxy.isAvailableBlocking() ) {

			auto methodCall = proxy.createMethodCall(memberID);

			if (sizeof ... (_InArgs) > 0) {
				SomeIPOutputStream outputStream = methodCall.getPayloadOutputStream();
				const bool success = SerializableArguments<_InArgs ...>::serialize(outputStream, inArgs ...);
				if (!success) {
					callStatus = CallStatus::OUT_OF_MEMORY;
					return;
				}
			}

			InputMessage messageReply = proxy.getConnection().sendMessageBlocking(methodCall);

			if ( messageReply.getHeader().isErrorType() ) {
				callStatus = CallStatus::REMOTE_ERROR;
			} else if (sizeof ... (_OutArgs) > 0) {
				SomeIPInputStream inputStream = messageReply.getPayloadInputStream();
				const bool success = SerializableArguments<_OutArgs ...>::deserialize(inputStream, outArgs ...);
				if (!success) {
					callStatus = CallStatus::REMOTE_ERROR;
					return;
				}
				callStatus = CallStatus::SUCCESS;
			}

		} else {
			log_warn( "Proxy is not available: ServiceID:%i", proxy.getServiceID() );
			callStatus = CallStatus::NOT_AVAILABLE;
		}
	}

	template<typename ... _ArgTypes>
	class AsyncCallbackHandler : public MessageSink {
public:
		typedef std::function<void (CallStatus, const _ArgTypes& ...)> FunctionType;

		AsyncCallbackHandler(FunctionType callback) :
			callback_(callback) {
		}

		~AsyncCallbackHandler() override {
		}

		virtual std::future<CallStatus> getFuture() {
			return promise_.get_future();
		}

		MessageProcessingResult processMessage(const InputMessage& message) override {
			handleReply( message, typename make_sequence<sizeof ... (_ArgTypes)>::type() );
			return MessageProcessingResult::Processed_OK;
		}

private:
		template<int ... _ArgIndices>
		inline CallStatus handleReply(
			const InputMessage& message, index_sequence<_ArgIndices ...>) {
			CommonAPI::CallStatus callStatus =
				(message.getHeader().isErrorType() ? CommonAPI::CallStatus::REMOTE_ERROR : CommonAPI::
				 CallStatus::SUCCESS);
			std::tuple<_ArgTypes ...> argTuple;

			if (callStatus == CallStatus::SUCCESS) {
				if ( !message.getHeader().isErrorType() ) {
					auto inputStream = message.getPayloadInputStream();
					const bool success = SerializableArguments<_ArgTypes ...>::deserialize(
						inputStream, std::get<_ArgIndices>(argTuple) ...);
					if (!success)
						callStatus = CallStatus::REMOTE_ERROR;
				} else {
					callStatus = CallStatus::REMOTE_ERROR;
				}
			}

			(void) argTuple;

			promise_.set_value(callStatus);

			callback_(callStatus, std::move( std::get<_ArgIndices>(argTuple) ) ...);
			return callStatus;
		}

		std::promise<CallStatus> promise_;
		const FunctionType callback_;
	};

	template<typename _ProxyType = SomeIPProxy, typename _AsyncCallback>
	static std::future<CallStatus> callMethodAsync(
		_ProxyType& proxy,
		MemberID memberID,
		const _InArgs& ... inArgs,
		_AsyncCallback asyncCallback) {

		if ( proxy.isAvailable() ) {

			auto message = proxy.createMethodCall(memberID);

			if (sizeof ... (_InArgs) > 0) {
				SomeIPOutputStream outputStream = message.getPayloadOutputStream();
				const bool success = SerializableArguments<_InArgs ...>::serialize(outputStream, inArgs ...);
				if (!success) {
					std::promise<CallStatus> promise;
					promise.set_value(CallStatus::OUT_OF_MEMORY);
					return promise.get_future();
				}
			}

			AsyncCallbackHandler<_OutArgs ...>* handler = new AsyncCallbackHandler<_OutArgs ...>(asyncCallback); // the object will be destroyed by the library

			proxy.getConnection().sendWithResponseHandler(message, *handler);

			return handler->getFuture();

		} else {

			CallStatus callStatus = CallStatus::NOT_AVAILABLE;

			log_warn( "Proxy is not available: ServiceID:%i", proxy.getServiceID() );

			callCallbackOnNotAvailable( asyncCallback, typename make_sequence<sizeof ... (_OutArgs)>::type() );

			std::promise<CallStatus> promise;
			promise.set_value(callStatus);
			return promise.get_future();
		}

	}

	template<int ... _ArgIndices>
	static void callCallbackOnNotAvailable(std::function<void(CallStatus, _OutArgs ...)> callback,
					       index_sequence<_ArgIndices ...>) {

		std::tuple<_OutArgs ...> argTuple;
		const CallStatus callstatus = CallStatus::NOT_AVAILABLE;
		callback(callstatus, std::move( std::get<_ArgIndices>(argTuple) ) ...);
		(void) argTuple;
	}
};

}
}
