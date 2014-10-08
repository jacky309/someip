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

		log_info() << "Client unregistered : " << toString();
	}

}

void Client::registerClient() {
	if (!m_registered) {
		getDispatcher().onNewClient(*this);
		m_registered = true;
	}
}

Service* Client::registerService(SomeIP::ServiceIDs serviceID, bool isLocal) {
	Service* service = m_dispatcher.tryRegisterService(serviceID, *this, isLocal);

	if (service != nullptr)
		m_registeredServices.push_back(service);

	return service;
}

void Client::unregisterService(SomeIP::ServiceIDs serviceID) {

	for (auto i = m_registeredServices.begin(); i != m_registeredServices.end(); ++i) {
		auto& service = *i;
		if (service->getServiceIDs() == serviceID) {
			log_debug() << "Unregistering service ID" << serviceID.toString() << " from " << toString();
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

void Client::subscribeToNotification(SomeIP::MemberIDs messageID) {
	log_debug() << "SUBSCRIBE_NOTIFICATION Message received from client " << toString() << ". MessageID:0x" << messageID.toString();
	auto& subscription = m_dispatcher.subscribeClientForNotifications(*this, messageID);
	m_subscribedNotifications.push_back(&subscription);
}

}
