#include "CommonAPI-SomeIP.h"
#include "StubAdapterHelper.h"

#include "SomeIP-common.h"

#include "SomeIPProxy.h"
#include "SomeIPRuntime.h"

using namespace CommonAPI::SomeIP;

namespace CommonAPI {
namespace SomeIP {

LOG_DECLARE_CONTEXT(someIPCommonAPILogContext, "SOCA", "SomeIP Common-API");

void SomeIPConnection::registerService(SomeIPStubAdapter& service) {
	m_connection.registerService( service.getServiceID() );
	m_serviceTable[service.getServiceID()] = &service;
	// TODO : return error code
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

	bool isRegistrationSuccessful = true;
	// TODO : handle error code
	registerService( *stubAdapter.get() );
	return isRegistrationSuccessful;

}


void SomeIPConnection::registerProxy(SomeIPProxy& proxy) {
	m_proxyTable[proxy.getServiceID()] = &proxy;
	getConnection().getServiceRegistry().registerServiceAvailabilityListener(proxy);
}

MessageProcessingResult SomeIPConnection::processMessage(const InputMessage& message) {

	auto serviceID = message.getHeader().getServiceID();
	if ( message.getHeader().isNotification() ) {

		// TODO : handle multiple proxy instances with same serviceID ?
		SomeIPProxy* proxy = m_proxyTable[serviceID];
		if (proxy != NULL)
			proxy->processMessage(message);

	} else if ( message.getHeader().isReply() ) {

		auto* messageHandler =
			m_messageHandlers[message.getHeader().getRequestID()];
		if (messageHandler != NULL) {
			messageHandler->processMessage(message);
			delete messageHandler;
		}

	} else {
		SomeIPStubAdapter* adapter = m_serviceTable[serviceID];
		if (adapter != NULL)
			adapter->processMessage(message);
	}
	return MessageProcessingResult::Processed_OK;
}

MessageProcessingResult SomeIPProxy::processMessage(const InputMessage& message) {

	// TODO : check what is done here
	auto handler = m_messageHandlers[message.getHeader().getMemberID()];
	if (handler != NULL) {
		handler->processMessage(message);
		return MessageProcessingResult::Processed_OK;
	}
	return MessageProcessingResult::NotProcessed_OK;
}

void SomeIPStubAdapter::init() {
	getConnection().registerService(*this);
	m_isRegistered = true;
}

SomeIPFunctionReturnCode SomeIPStubAdapter::sendMessage(OutputMessage& msg) {
	return getConnection().sendMessage(msg);
}

void SomeIPStubAdapter::deinit() {
	if(m_isRegistered) {
		getConnection().unregisterService( getServiceID() );
		m_isRegistered = false;
	}
}

}

std::unordered_map<std::string, SomeIPFactory::ProxyFactoryFunction> SomeIPFactory::registeredProxyFactoryFunctions;
std::unordered_map<std::string, SomeIPFactory::AdapterFactoryFunction> SomeIPFactory::registeredAdapterFactoryFunctions;

void SomeIPFactory::registerAdapterFactoryMethod(const std::string& interfaceID, AdapterFactoryFunction function) {
	log_info("Registering stubadapter factory for interface: ") << interfaceID;
	registeredAdapterFactoryFunctions.insert({interfaceID, function});
}

void SomeIPFactory::registerProxyFactoryMethod(const std::string& interfaceID, ProxyFactoryFunction function) {
	log_info("Registering proxy factory for interface: ") << interfaceID;
	registeredProxyFactoryFunctions.insert({interfaceID, function});
}

}
