#include "Dispatcher.h"
#include "SomeIP-common.h"

#include "GlibIO.h"
#include "ServiceDiscovery.h"

namespace SomeIP_Dispatcher {

LOG_DECLARE_DEFAULT_CONTEXT(dispatcherContext, "disp", "Dispatcher");

void Notification::sendMessageToSubscribedClients(const DispatcherMessage& msg) {
	for (auto* client : m_subscribedClients)
		client->sendMessage(msg);
}

void Dispatcher::dispatchMessage(DispatcherMessage& msg, Client& client) {

	if (m_messageCounter++ % 10000 == 0)
		log_info() << "Message count : " << m_messageCounter;

	log_traffic() << "Message received from client " << client.toString() << " : " << msg;

	auto& header = msg.getHeader();

	if ( header.isNotification() ) {

		if (header.getMessageID() == SomeIPServiceDiscoveryMessage::SERVICE_DISCOVERY_MEMBER_ID) {
			assert(false);
		} else {
			// we are forwarding a notification to several clients
			Notification& notification = getOrCreateNotification( MemberIDs(header.getServiceID(), header.getInstanceID(), header.getMemberID() ) );
			notification.sendMessageToSubscribedClients(msg);
		}

	} else if ( header.isReply() ) {
		auto clientIdentifier = msg.getClientIdentifier();
		Client* client = getClientFromId(clientIdentifier);

		if (client == nullptr) {
			log_warning() << "Answer to client can not be sent since the client has disconnected. client ID: " <<
			clientIdentifier;
		} else {
			client->sendMessage(msg);
		}

	} else {

		bool bServiceFound = false;

		// If needed, identify the client can that the answer can be sent to it
		if ( header.isRequestWithReturn() ) {
			msg.setClientIdentifier( client.getIdentifier() );
		}

		// service request
		for (auto& service : m_services) {
			if ( service->matchesRequest( msg ) ) {
				service->sendMessage(msg);
				bServiceFound = true;
			}
		}

		if (!bServiceFound) {
			OutputMessage responseMsg = createMethodReturn(msg);
			responseMsg.getHeader().setMessageType(SomeIP::MessageType::ERROR);
			responseMsg.setClientIdentifier(-1);
			client.sendMessage(responseMsg);
		}

	}
}

std::string Dispatcher::dumpState() {

	std::string s = "Notifications:\n";
	for (auto& notification : m_notifications) {
		s += notification->toString().c_str();
		s += "\n";
	}

	s += "-------------- \nServices:\n";
	for (auto& service : m_services) {
		s += service->toString().c_str();
		s += "\n";
	}

	s += "-------------- \nClients:\n";
	for (auto& client : m_clients) {
		if (client != nullptr) {
			s += client->toString().c_str();
			s += "\n";
		}
	}

	return s;
}

void Dispatcher::cleanDisconnectedClients() {

	for (auto client : m_disconnectedClients) {
		delete client;
	}

	m_disconnectedClients.resize(0);
}

Service* Dispatcher::tryRegisterService(SomeIP::ServiceIDs serviceID, Client& client, bool isLocal) {

	log_info() << "tryRegisterService " << serviceID.toString() << " from " << client.toString();
	Service* service = getService(serviceID);

	if (service != nullptr) {
		// we are trying to register a well known or existing service
		if (isLocal) {
			if (service->setClient(client) == ReturnCode::OK) {
				return service;
			}
		} else {
			// remote service with a matching local service, or already known => don't register
			return nullptr;
		}
	} else {

		// The client is registering a new service
		service = new Service(serviceID, isLocal);
		service->setClient(client);

		if (registerService(*service) != ReturnCode::OK) {
			delete service;
			return nullptr;
		}
	}

	if (service->getClient() != &client) {
		log_warn() << "registration refused : " << serviceID.toString();
		return nullptr;
	}

	return service;
}

ReturnCode Dispatcher::registerService(Service& service) {

	for (auto& existingService : m_services) {
		if ( existingService->isDuplicate( service.getServiceIDs() ) ) {
			log_error() << "Service already registered : " << existingService->toString();
			return ReturnCode::DUPLICATE;
		}
	}

	m_services.push_back(&service);

	log_info() << "Service registered : " << service.toString();

	// Notify clients
	for (auto client : m_serviceRegistrationListeners) {
		client->onServiceRegistered(service);
	}

	for (auto notification : m_notifications) {
		if ( notification->matchesServiceID( service.getServiceIDs() ) )
			notification->setProviderService(&service);
	}

	return ReturnCode::OK;
}

void Dispatcher::unregisterService(Service& service) {
	removeFromVector(m_services, &service);

	// Notify listeners in reverse order since the service announcer needs to be notified before having the service unregistered from TCP and UDP endpoints
	for (auto i = m_serviceRegistrationListeners.rbegin(); i != m_serviceRegistrationListeners.rend(); ++i) {
		auto listener = *i;
		listener->onServiceUnregistered(service);
	}

	for (auto& notification : m_notifications) {
		if (notification->getProviderService() == &service)
			notification->setProviderService(nullptr);
	}

	log_info() << "Service unregistered : " << service.toString();
}

void Dispatcher::sendPingMessages() {
	for (auto& client : m_clients)
		if (client != nullptr) {
			if ( client->isConnected() )
				client->sendPingMessage();
		}
}

void Dispatcher::onNewClient(Client& client) {
	client.setIdentifier(m_nextAvailableClientID++);
	assert( client.getIdentifier() == m_clients.size() );
	m_clients.push_back(&client);
	client.init();
}

void Dispatcher::onClientDisconnected(Client& client) {
	assert(m_clients[client.getIdentifier()] != nullptr);
	m_clients[client.getIdentifier()] = nullptr;
	m_disconnectedClients.push_back(&client);
	m_idleCallback->activate();
}

std::string Notification::toString() const {
	StringBuilder s;
	s << "Notification MessageID: " << getMessageID().toString();

	if (m_providerService != nullptr)
		s << "/ Service:" << m_providerService->toString();

	if (m_subscribedClients.size() != 0) {
		s << "/ Notified: ";
		for (auto i = m_subscribedClients.begin(); i < m_subscribedClients.end(); i++) {
			auto& client = *i;
			s << client->toString() << ", ";
		}
	}
	return s;
}

std::string Service::toString() const {
	return (logging::StringBuilder() << "Service ServiceID: "<< getServiceIDs().toString() <<
			", " << ( (m_client != nullptr) ? m_client->toString() : "Unknown client") );
}

void Service::sendMessage(const DispatcherMessage& msg) {
	m_client->sendMessage(msg);
}

void Service::onNotificationSubscribed(SomeIP::MemberID memberID) {
	if (getClient() != nullptr)
		getClient()->onNotificationSubscribed(*this, memberID);
}

void Notification::unsubscribe(Client& clientToUnsubscribe) {
	for (auto i = m_subscribedClients.begin(); i < m_subscribedClients.end(); i++) {
		Client* client = *i;
		if (client == &clientToUnsubscribe) {
			m_subscribedClients.erase(i);
			log_debug( ) << "Client un-subscribed to " << toString() << " : " << client->toString();
			break;
		}
	}
}

void Notification::subscribe(Client& client) {
	m_subscribedClients.push_back(&client);
	if (m_providerService != nullptr)
		m_providerService->onNotificationSubscribed( m_messageID.m_memberID );
}

void Notification::init() {
	auto service = m_dispatcher.getService( m_messageID.m_serviceIDs );
	if (service != nullptr)
		setProviderService(service);
}

void Notification::setProviderService(Service* service) {
	m_providerService = service;
	if (m_providerService != nullptr)
		m_providerService->onNotificationSubscribed(m_messageID.m_memberID);
}

//ClientIdentifier Client::s_nextAvailableID = 0;

}
