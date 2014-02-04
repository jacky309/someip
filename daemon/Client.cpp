#include "Client.h"

namespace SomeIP_Dispatcher {

void Client::unregisterClient() {

	if (m_registered) {
		for (auto* subscription : m_subscribedNotifications)
			subscription->unsubscribe(*this);

		for (auto* service : m_registeredServices)
			m_dispatcher.unregisterService(*service);

		getDispatcher().onClientDisconnected(*this);

		m_registered = false;

		log_info( "Client unregistered %s", toString().c_str() );
	}

}

void Client::registerClient() {
	if (!m_registered) {
		getDispatcher().onNewClient(*this);
		m_registered = true;
	}
}

Service* Client::registerService(SomeIP::ServiceID serviceID, bool isLocal) {
	Service* service = m_dispatcher.tryRegisterService(serviceID, *this, isLocal);

	if (service != NULL)
		m_registeredServices.push_back(service);

	return service;
}

void Client::unregisterService(SomeIP::ServiceID serviceID) {

	for (auto i = m_registeredServices.begin(); i != m_registeredServices.end(); ++i) {
		auto& service = *i;
		if (service->getServiceID() == serviceID) {
			log_debug("Unregistering service ID") << serviceID << " from " << toString();
			getDispatcher().unregisterService(*service);
			m_registeredServices.erase(i);

			// TODO : delete service
			break;
		}
	}

}

void Client::processIncomingMessage(InputMessage& msg) {
	if (m_endPoint.processMessage(msg) == MessageProcessingResult::NotProcessed_OK)
		if (m_pingSender.processMessage(msg) == MessageProcessingResult::NotProcessed_OK)
			getDispatcher().dispatchMessage(msg, *this);
}

void Client::subscribeToNotification(SomeIP::MessageID messageID) {
	log_debug("SUBSCRIBE_NOTIFICATION Message received from client %s. MessageID:0x%X", toString().c_str(), messageID);
	auto& subscription = m_dispatcher.subscribeClientForNotifications(*this, messageID);
	m_subscribedNotifications.push_back(&subscription);
}

}
