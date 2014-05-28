#pragma once

//#include "CommonAPI/CommonAPI.h"
#include "CommonAPI/Factory.h"

#include "CommonAPI-SomeIP.h"
#include "SerializableArguments.h"
#include "SomeIPHelper.h"

#include <memory>
#include <initializer_list>
#include <tuple>
#include <unordered_map>

#include "SomeIP-common.h"
#include "SomeIP-clientLib.h"
#include "SomeIPRuntime.h"


namespace CommonAPI {
namespace SomeIP {


template<typename _StubClass>
class StubAdapterHelper : public SomeIPStubAdapter,
	public std::enable_shared_from_this<typename _StubClass::StubAdapterType> {
public:
	typedef typename _StubClass::StubAdapterType StubAdapterType;
	typedef typename _StubClass::RemoteEventHandlerType RemoteEventHandlerType;

	class StubDispatcher {
public:
		virtual ~StubDispatcher() {
		}
		virtual bool dispatchMessage(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
					     StubAdapterHelper<_StubClass>& adapterHelper) = 0;
	};

public:
	StubAdapterHelper(const std::shared_ptr<_StubClass>& stub, SomeIPConnection& connection,
			  const std::string& commonApiAddress,
			  CommonAPI::SomeIP::ServiceID serviceID) :
		SomeIPStubAdapter(commonApiAddress, serviceID, connection), stub_(stub) {
		remoteEventHandler_ = NULL;
	}

	virtual ~StubAdapterHelper() {
		stub_.reset();
	}

	void init() override {
		SomeIPStubAdapter::init();
		remoteEventHandler_ = stub_->initStubAdapter( getStubAdapter() );
	}

	void deinit() override {
		SomeIPStubAdapter::deinit();
		stub_.reset();
	}

	inline std::shared_ptr<StubAdapterType> getStubAdapter() {
		return this->shared_from_this();
	}

	inline RemoteEventHandlerType* getRemoteEventHandler() {
		return remoteEventHandler_;
	}

protected:
	// interfaceMemberName, interfaceMemberSignature
	typedef MemberID SomeIPInterfaceMemberPath;
	typedef std::unordered_map<SomeIPInterfaceMemberPath, StubDispatcher*> StubDispatcherTable;

	MessageProcessingResult processMessage(const InputMessage& msg) override {

		if (SomeIPStubAdapter::processMessage(msg) == MessageProcessingResult::Processed_OK)
			return MessageProcessingResult::Processed_OK;

		auto interfaceMemberName = msg.getHeader().getMemberID();
		auto findIterator = this->stubDispatcherTable_.find(interfaceMemberName);
		const bool foundInterfaceMemberHandler = ( findIterator != stubDispatcherTable_.end() );
		auto messageHandled = MessageProcessingResult::Processed_Error;

		//To prevent the destruction of the stub whilst still handling a message
		auto stubSafety = stub_;
		if (stubSafety && foundInterfaceMemberHandler) {
			StubDispatcher* stubDispatcher = findIterator->second;
			messageHandled =
				(stubDispatcher->dispatchMessage(msg, stubSafety,
								 *this) ? MessageProcessingResult::Processed_OK :
				 MessageProcessingResult::Processed_Error);
		} else {
			log_error("Can not find member with MemberID=0x%X", interfaceMemberName);
			log_info() << "Available member IDs:";
			for (auto& i : stubDispatcherTable_) {
				log_info().writeFormatted("0x%X", i.first);
			}
		}

		return messageHandled;
	}

	std::shared_ptr<_StubClass> stub_;
	RemoteEventHandlerType* remoteEventHandler_;
	static const StubDispatcherTable stubDispatcherTable_;
};

template<class>
struct StubSignalHelper;

template<template<class ...> class _In, class ... _InArgs>
struct StubSignalHelper<_In<_InArgs ...> > {
	template<typename _SomeIPStub = SomeIPStubAdapter>
	static bool sendSignal(const _SomeIPStub& stub,
			       MemberID id,
			       const _InArgs& ... inArgs) {

		OutputMessage message( SomeIP::getMessageID(stub.getServiceID(), id) );

		message.getHeader().setMessageType(MessageType::NOTIFICATION);

		if (sizeof ... (_InArgs) > 0) {
			SomeIPOutputStream outputStream = message.getPayloadOutputStream();
			const bool success = SerializableArguments<_InArgs ...>::serialize(outputStream, inArgs ...);
			if (!success) {
				return false;
			}
		}

		return !isError( stub.getConnection().sendMessage(message) );
	}
};

template<class, class> class SomeIPMethodStubDispatcher;

template<typename _StubClass, template<class ...> class _In, class ... _InArgs>
class SomeIPMethodStubDispatcher<_StubClass, _In<_InArgs ...> >
	: public StubAdapterHelper<_StubClass>::StubDispatcher {
public:
	typedef StubAdapterHelper<_StubClass> StubAdapterHelperType;
	typedef void (_StubClass::* _StubFunctor)(_InArgs ...);

	SomeIPMethodStubDispatcher(_StubFunctor stubFunctor) : stubFunctor_(stubFunctor) {
	}

	bool dispatchMessage(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
			     StubAdapterHelperType& stubAdapterHelper) {
		return handleMessage( message, stub, stubAdapterHelper, typename make_sequence<sizeof ... (_InArgs)>::type() );
	}

private:
	template<int ... _InArgIndices, int ... _OutArgIndices>
	inline bool handleMessage(const InputMessage& message,
				  const std::shared_ptr<_StubClass>& stub,
				  StubAdapterHelperType& stubAdapterHelper,
				  index_sequence<_InArgIndices ...>) const {
		std::tuple<_InArgs ...> argTuple;

		if (sizeof ... (_InArgs) > 0) {
			auto inputStream = message.getPayloadInputStream();
			const bool success =
				SerializableArguments<_InArgs ...>::deserialize(inputStream, std::get<_InArgIndices>(
											argTuple) ...);
			if (!success)
				return false;
		}

		(stub.get()->*stubFunctor_)(std::move( std::get<_InArgIndices>(argTuple) ) ...);

		return true;
	}

	_StubFunctor stubFunctor_;
};

template<class, class, class>
class MethodWithReplyStubDispatcher;

template<typename _StubClass, template<class ...> class _In, class ... _InArgs,
	 template<class ...> class _Out, class ... _OutArgs>
class MethodWithReplyStubDispatcher<_StubClass, _In<_InArgs ...>, _Out<_OutArgs ...> >:
	public StubAdapterHelper<_StubClass>::StubDispatcher {
public:
	typedef StubAdapterHelper<_StubClass> StubAdapterHelperType;
	typedef void (_StubClass::* _StubFunctor)(const std::shared_ptr<CommonAPI::ClientId> clientId, _InArgs ...,
						  _OutArgs& ...);

	MethodWithReplyStubDispatcher(_StubFunctor stubFunctor) :
		stubFunctor_(stubFunctor) {
	}

	bool dispatchMessage(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
			     StubAdapterHelperType& stubAdapterHelper) {
		return handleMessage(
			       message,
			       stub,
			       stubAdapterHelper,
			       typename make_sequence_range<sizeof ... (_InArgs), 0>::type(),
			       typename make_sequence_range<sizeof ... (_OutArgs), sizeof ... (_InArgs)>::type() );
	}

private:
	template<int ... _InArgIndices, int ... _OutArgIndices>
	inline bool handleMessage(const InputMessage& message,
				  const std::shared_ptr<_StubClass>& stub,
				  StubAdapterHelperType& stubAdapterHelper,
				  index_sequence<_InArgIndices ...>,
				  index_sequence<_OutArgIndices ...>) const {
		std::tuple<_InArgs ..., _OutArgs ...> argTuple;

		if (sizeof ... (_InArgs) > 0) {
			auto inputStream = message.getPayloadInputStream();
			const bool success =
				SerializableArguments<_InArgs ...>::deserialize(inputStream, std::get<_InArgIndices>(
											argTuple) ...);
			if ( inputStream.hasMoreData() )
				log_warning() << "Payload was not read completely. Message : " << message.toString();
			if (!success)
				return false;
		}
		(void) argTuple;

		(stub.get()->*stubFunctor_)(getSomeIPClientID(message), std::move( std::get<_InArgIndices>(argTuple) ) ...,
					    std::get<_OutArgIndices>(argTuple) ...);

		OutputMessage messageReply = createMethodReturn(message);

		if (sizeof ... (_OutArgs) > 0) {
			SomeIPOutputStream outputStream = messageReply.getPayloadOutputStream();
			const bool success =
				SerializableArguments<_OutArgs ...>::serialize(outputStream, std::get<_OutArgIndices>(
										       argTuple) ...);
			if (!success)
				return false;

		}

		return !isError( stubAdapterHelper.getConnection().sendMessage(messageReply) );
	}

	_StubFunctor stubFunctor_;
};

template<typename _StubClass, typename _AttributeType>
class GetAttributeStubDispatcher : public StubAdapterHelper<_StubClass>::StubDispatcher {
public:
	typedef StubAdapterHelper<_StubClass> StubAdapterHelperType;
	typedef const _AttributeType& (_StubClass::* GetStubFunctor)(const std::shared_ptr<CommonAPI::ClientId> clientId);

	GetAttributeStubDispatcher(GetStubFunctor getStubFunctor) :
		getStubFunctor_(getStubFunctor) {
	}

	bool dispatchMessage(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
			     StubAdapterHelperType& stubAdapterHelper) {
		return sendAttributeValueReply(message, stub, stubAdapterHelper);
	}

protected:
	inline bool sendAttributeValueReply(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
					    StubAdapterHelperType& stubAdapterHelper) {
		OutputMessage messageReply = createMethodReturn(message);
		SomeIPOutputStream outputStream = messageReply.getPayloadOutputStream();

		outputStream << (stub.get()->*getStubFunctor_)( getSomeIPClientID(message) );

		return !isError( stubAdapterHelper.getConnection().sendMessage(messageReply) );
	}

	GetStubFunctor getStubFunctor_;
};

template<typename _StubClass, typename _AttributeType>
class SetAttributeStubDispatcher : public GetAttributeStubDispatcher<_StubClass, _AttributeType> {
public:
	typedef typename GetAttributeStubDispatcher<_StubClass, _AttributeType>::StubAdapterHelperType StubAdapterHelperType;
	typedef typename StubAdapterHelperType::RemoteEventHandlerType RemoteEventHandlerType;

	typedef typename GetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
	typedef bool (RemoteEventHandlerType::* OnRemoteSetFunctor)(const std::shared_ptr<CommonAPI::ClientId> clientId,
								    _AttributeType value);
	typedef void (RemoteEventHandlerType::* OnRemoteChangedFunctor)();

	SetAttributeStubDispatcher(GetStubFunctor getStubFunctor, OnRemoteSetFunctor onRemoteSetFunctor,
				   OnRemoteChangedFunctor onRemoteChangedFunctor) :
		GetAttributeStubDispatcher<_StubClass,
					   _AttributeType>(getStubFunctor), onRemoteSetFunctor_(onRemoteSetFunctor),
		onRemoteChangedFunctor_(
			onRemoteChangedFunctor) {
	}

	bool dispatchMessage(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
			     StubAdapterHelperType& stubAdapterHelper) {
		bool attributeValueChanged;

		if ( !setAttributeValue(message, stub, stubAdapterHelper, attributeValueChanged) )
			return false;

		if (attributeValueChanged)
			notifyOnRemoteChanged(stubAdapterHelper);

		return true;
	}

protected:
	inline bool setAttributeValue(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
				      StubAdapterHelperType& stubAdapterHelper, bool& attributeValueChanged) {
		auto inputStream = message.getPayloadInputStream();
		_AttributeType attributeValue;
		inputStream >> attributeValue;
		if ( inputStream.hasError() )
			return false;

		attributeValueChanged =
			(stubAdapterHelper.getRemoteEventHandler()->*onRemoteSetFunctor_)( getSomeIPClientID(message),
											   std::move(attributeValue) );

		return this->sendAttributeValueReply(message, stub, stubAdapterHelper);
	}

	inline void notifyOnRemoteChanged(StubAdapterHelperType& stubAdapterHelper) {
		(stubAdapterHelper.getRemoteEventHandler()->*onRemoteChangedFunctor_)();
	}

	inline const _AttributeType& getAttributeValue(std::shared_ptr<CommonAPI::ClientId> clientId,
						       const std::shared_ptr<_StubClass>& stub) {
		return ( stub.get()->*(this->getStubFunctor_) )(clientId);
	}

	const OnRemoteSetFunctor onRemoteSetFunctor_;
	const OnRemoteChangedFunctor onRemoteChangedFunctor_;
};

template<typename _StubClass, typename _AttributeType>
class SetObservableAttributeStubDispatcher : public SetAttributeStubDispatcher<_StubClass, _AttributeType> {
public:
	typedef typename SetAttributeStubDispatcher<_StubClass, _AttributeType>::StubAdapterHelperType StubAdapterHelperType;
	typedef typename StubAdapterHelperType::StubAdapterType StubAdapterType;

	typedef typename SetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
	typedef typename SetAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteSetFunctor OnRemoteSetFunctor;
	typedef typename SetAttributeStubDispatcher<_StubClass,
						    _AttributeType>::OnRemoteChangedFunctor OnRemoteChangedFunctor;
	typedef void (StubAdapterType::* FireChangedFunctor)(const _AttributeType&);

	SetObservableAttributeStubDispatcher(GetStubFunctor getStubFunctor, OnRemoteSetFunctor onRemoteSetFunctor,
					     OnRemoteChangedFunctor onRemoteChangedFunctor,
					     FireChangedFunctor fireChangedFunctor) :
		SetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor, onRemoteSetFunctor,
								       onRemoteChangedFunctor), fireChangedFunctor_(
			fireChangedFunctor) {
	}

	bool dispatchMessage(const InputMessage& message, const std::shared_ptr<_StubClass>& stub,
			     StubAdapterHelperType& stubAdapterHelper) {
		bool attributeValueChanged;
		if ( !this->setAttributeValue(message, stub, stubAdapterHelper, attributeValueChanged) )
			return false;

		if (attributeValueChanged) {
			fireAttributeValueChanged( stubAdapterHelper, stub, getSomeIPClientID(message) );
			this->notifyOnRemoteChanged(stubAdapterHelper);
		}
		return true;
	}

private:
	inline void fireAttributeValueChanged(StubAdapterHelperType& stubAdapterHelper,
					      const std::shared_ptr<_StubClass> stub, const std::shared_ptr<ClientId> client) {
		(stubAdapterHelper.getStubAdapter().get()->*fireChangedFunctor_)( this->getAttributeValue(client, stub) );
	}

	const FireChangedFunctor fireChangedFunctor_;
};

}     // namespace SomeIP
} // namespace CommonAPI
