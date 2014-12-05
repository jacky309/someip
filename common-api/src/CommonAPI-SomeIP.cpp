#include "CommonAPI-SomeIP.h"
#include "StubAdapterHelper.h"

#include "SomeIP-common.h"

#include "SomeIPProxy.h"
#include "SomeIPRuntime.h"

using namespace CommonAPI::SomeIP;

namespace CommonAPI {
namespace SomeIP {

LOG_DECLARE_CONTEXT(someIPCommonAPILogContext, "SOCA", "SomeIP Common-API");

SomeIPReturnCode SomeIPConnection::registerService(SomeIPStubAdapter& service) {
	auto code = getConnection().registerService( service.getServiceIDs() );

	if ( !isError(code) )
		m_serviceTable[service.getServiceIDs()] = &service;

	return code;
}

bool SomeIPConnection::registerService(const std::shared_ptr<StubBase>& stubBase,
				       const char* interfaceId,
				       const std::string& participantId,
				       const std::string& serviceName,
				       const std::string& domain,
				       const std::shared_ptr<Factory>& factory) {

	auto someIPFactory = static_cast<SomeIPFactory*>( factory.get() );
	assert(someIPFactory != nullptr);

	auto stubAdapter = someIPFactory->createAdapter(stubBase, interfaceId, participantId, serviceName, domain);
	if (!stubAdapter) {
		return false;
	}

	auto code = registerService( *stubAdapter.get() );

	return !isError(code);

}

SomeIPReturnCode SomeIPConnection::registerProxy(SomeIPProxy& proxy) {
	m_proxyTable[proxy.getServiceIDs()] = &proxy;
	getConnection().getServiceRegistry().registerServiceAvailabilityListener(proxy);

	return SomeIPReturnCode::OK;
}

MessageProcessingResult SomeIPConnection::processMessage(const InputMessage& message) {

	ServiceIDs serviceID(message.getServiceID(), message.getInstanceID());

	if ( message.getHeader().isNotification() ) {

		// TODO : handle multiple proxy instances with same serviceID ?
		SomeIPProxy* proxy = m_proxyTable[serviceID];
		if (proxy != nullptr)
			proxy->processMessage(message);

	} else if ( message.getHeader().isReply() ) {

		auto* messageHandler =
			m_messageHandlers[message.getHeader().getRequestID()];
		if (messageHandler != nullptr) {
			messageHandler->processMessage(message);
			delete messageHandler;
		}

	} else {
		SomeIPStubAdapter* adapter = m_serviceTable[serviceID];
		if (adapter != nullptr)
		{
			auto result = adapter->processMessage(message);
			if (result != MessageProcessingResult::Processed_OK) {
				// The adapter could not process the message => send an error back to the sender
				OutputMessage errorMessage = createMethodReturn(message);
				errorMessage.getHeader().setMessageType(MessageType::ERROR);
				sendMessage(errorMessage);
			}
		}
	}
	return MessageProcessingResult::Processed_OK;
}

MessageProcessingResult SomeIPProxy::processMessage(const InputMessage& message) {

	// TODO : check what is done here
	auto handler = m_messageHandlers[message.getHeader().getMemberID()];
	if (handler != nullptr) {
		handler->processMessage(message);
		return MessageProcessingResult::Processed_OK;
	}
	return MessageProcessingResult::NotProcessed_OK;
}

void SomeIPStubAdapter::init() {
	//	getConnection().registerService(*this);
	//	m_isRegistered = true;
}

SomeIPReturnCode SomeIPStubAdapter::sendMessage(const OutputMessage& msg) {
	return getConnection().sendMessage(msg);
}

void SomeIPStubAdapter::deinit() {
	//	if(m_isRegistered) {
	//		getConnection().unregisterService( getServiceID() );
	//		m_isRegistered = false;
	//	}
}

}

void SomeIPRuntime::registerAdapterFactoryMethod(const std::string& interfaceID, AdapterFactoryFunction function) {
	printf("SomeIP: Registering stubadapter factory for interface: %s\n", interfaceID.c_str());
	registeredAdapterFactoryFunctions.insert({interfaceID, function});
}

void SomeIPRuntime::registerProxyFactoryMethod(const std::string& interfaceID, ProxyFactoryFunction function) {
	printf("SomeIP: Registering proxy factory for interface: %s\n", interfaceID.c_str());
	registeredProxyFactoryFunctions.insert({interfaceID, function});
}


std::shared_ptr<Factory> SomeIPRuntime::doCreateFactory(std::shared_ptr<MainLoopContext> mainLoopContext,
					 const std::string& factoryName,
					 const bool nullOnInvalidName) {
//		m_connection = std::make_shared<SomeIPConnection>( new SomeIPClient::ClientDaemonConnection() );

	m_connection = std::make_shared<SomeIPConnection>();

//		MainLoopInterface* mainLoop = new CommonAPIMainLoopInterface(mainLoopContext);

//	m_connection->setConnection(new SomeIPClient::DaemonLessClient(*m_connection));
	m_connection->setConnection(new SomeIPClient::ClientDaemonConnection());

	return createFactory(mainLoopContext);
}


}
