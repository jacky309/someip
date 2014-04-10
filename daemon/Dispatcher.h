#pragma once

#include "Client.h"
#include "SomeIP.h"
#include <vector>
#include <algorithm>
#include "config.h"
#include "Message.h"
#include "ipc.h"
#include "SomeIP-common.h"
#include "GlibIO.h"

namespace SomeIP_Dispatcher {

using namespace std;

class Service;
class Client;
class ServiceRegistrationListener;
class Dispatcher;

enum class ReturnCode {
	OK,
	DUPLICATE,
	ERROR
};

/**
 * This class manages the subscription/un-subscription of clients against properties
 */
class Notification {

public:
	Notification(Dispatcher& dispatcher, SomeIP::MessageID messageID) :
		m_messageID(messageID), m_dispatcher(dispatcher) {
		init();
	}

	void init();

	void subscribe(Client& client);

	void unsubscribe(Client& clientToUnsubscribe);

	void sendMessageToSubscribedClients(const DispatcherMessage& msg);

	SomeIP::MessageID getMessageID() const {
		return m_messageID;
	}

	void setProviderService(Service* service);

	Service* getProviderService() {
		return m_providerService;
	}

	bool matchesServiceID(SomeIP::ServiceID serviceID) {
		return ( serviceID == SomeIP::getServiceID(m_messageID) );
	}

	std::string toString() const;

private:
	vector<Client*> m_subscribedClients;
	const SomeIP::MessageID m_messageID;
	Service* m_providerService = nullptr;
	Dispatcher& m_dispatcher;

};

/**
 * Represents a service registered on the dispatcher
 */
class Service : public SomeIP::SomeIPService {
public:
	Service(SomeIP::ServiceID serviceID, bool isLocal) :
		SomeIPService(serviceID), m_isLocal(isLocal) {
	}

	virtual ~Service() {
	}

	Client* getClient() {
		return m_client;
	}

	virtual ReturnCode setClient(Client& client) {
		if (m_client == NULL) {
			m_client = &client;
			return ReturnCode::OK;
		} else
			return ReturnCode::ERROR;
	}

	/**
	 * Returns true if the service is provided by a local application, or false otherwise
	 */
	bool isLocal() const {
		return m_isLocal;
	}

	virtual void sendMessage(DispatcherMessage& msg);

	virtual void onNotificationSubscribed(SomeIP::MemberID memberID);

	virtual std::string toString() const;

	Client* m_client = NULL;

	bool m_isLocal;

};

/**
 * Main dispatcher class
 */
class Dispatcher {

	static const int PING_DELAY = 5000;

public:
	Dispatcher() :
		m_idleCallback([&]() {
				       cleanDisconnectedClients();
				       return false;
			       }), m_pingTimer([&]() {
						       sendPingMessages();
					       }, PING_DELAY) {
		m_messageCounter = 0;
	}

	void dispatchMessage(DispatcherMessage& msg, Client& client);

	Notification& subscribeClientForNotifications(Client& client, SomeIP::MessageID messageID) {
		Notification& notification = getOrCreateNotification(messageID);
		notification.subscribe(client);
		return notification;
	}

	std::string dumpState();

	void cleanDisconnectedClients();

	Notification& getOrCreateNotification(SomeIP::MessageID messageID) {

		for (auto notification : m_notifications) {
			if (notification->getMessageID() == messageID)
				return *notification;
		}

		// no existing notification found => add a new one. TODO : destroy instance
		Notification* newNotification = new Notification(*this, messageID);
		m_notifications.push_back(newNotification);

		return *newNotification;
	}

	Client* getClientFromId(ClientIdentifier id) {
		Client* client = NULL;
		if (m_clients.size() > id) {
			client = m_clients[id];
		}

		return client;
	}

	const vector<Service*> getServices() {
		return m_services;
	}

	void onNewClient(Client& client);

	void addServiceRegistrationListener(ServiceRegistrationListener& listener) {
		m_serviceRegistrationListeners.push_back(&listener);
	}

	void removeServiceRegistrationListener(ServiceRegistrationListener& listener) {
		removeFromVector(m_serviceRegistrationListeners, &listener);
	}

	void onClientDisconnected(Client& client);

	Service* tryRegisterService(SomeIP::ServiceID serviceID, Client& client, bool isLocal = true);
	ReturnCode registerService(Service& service);

	void unregisterService(Service& service);

	Service* getService(SomeIP::ServiceID serviceID) {
		for (auto& service : m_services) {
			if (service->getServiceID() == serviceID)
				return service;
		}
		return NULL;
	}

	void sendPingMessages();

private:
	vector<Notification*> m_notifications;
	vector<Service*> m_services;
	vector<Client*> m_clients;
	vector<Client*> m_disconnectedClients;
	vector<ServiceRegistrationListener*> m_serviceRegistrationListeners;
	int m_messageCounter;
	GlibIdleCallback m_idleCallback;
	GLibTimer m_pingTimer;

};

//void trace_message(const DispatcherMessage& msg);

}
