#pragma once

#include "Client.h"
#include "SomeIP.h"
#include <vector>
#include <algorithm>
#include "Message.h"
#include "ipc.h"
#include "SomeIP-common.h"
#include "GlibIO.h"
#include "Networking.h"
#include <unordered_map>

namespace SomeIP_Dispatcher {

typedef std::unordered_map<ServiceID, InstanceID> ServiceInstanceNamespace;

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

	Notification(Dispatcher& dispatcher, SomeIP::MemberIDs messageID) :
		m_messageID(messageID), m_dispatcher(dispatcher) {
		init();
	}

	void init();

	void subscribe(Client& client);

	void unsubscribe(Client& clientToUnsubscribe);

	void sendMessageToSubscribedClients(const DispatcherMessage& msg);

	const SomeIP::MemberIDs& getMessageID() const {
		return m_messageID;
	}

	void setProviderService(Service* service);

	Service* getProviderService() {
		return m_providerService;
	}

	bool matchesServiceID(SomeIP::ServiceIDs serviceID) const {
		return ( serviceID == m_messageID.m_serviceIDs );
	}

	std::string toString() const;

private:
	vector<Client*> m_subscribedClients;
	MemberIDs m_messageID;
	Service* m_providerService = nullptr;
	Dispatcher& m_dispatcher;

};

/**
 * Represents a service registered on the dispatcher
 */
class Service : public SomeIP::SomeIPService {
public:
	Service(SomeIP::ServiceIDs serviceID, bool isLocal) :
		SomeIPService(serviceID), m_isLocal(isLocal) {
	}

	virtual ~Service() {
	}

	Client* getClient() {
		return m_client;
	}

	const Client* getClient() const {
		return m_client;
	}

	virtual ReturnCode setClient(Client& client) {
		if (m_client == nullptr) {
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

	/**
	 * Returns true if the service already has a
	 */
	virtual bool isAssigned() {
		return true;
	}

	virtual void sendMessage(const DispatcherMessage& msg);

	virtual void onNotificationSubscribed(SomeIP::MemberID memberID);

	virtual std::string toString() const;

private:
	Client* m_client = nullptr;

	bool m_isLocal;

};

/**
 * Main dispatcher class
 */
class Dispatcher {

	static const int PING_DELAY = 5000;

public:
	Dispatcher(MainLoopContext& mainLoopContext) :
		m_idleCallback( mainLoopContext.addIdleCallback([&]() {
									cleanDisconnectedClients();
									return false;
								}) ), m_pingTimer( mainLoopContext.addTimeout([&]() {
														      sendPingMessages();
													      }, PING_DELAY) ) {
		m_messageCounter = 0;
	}

	void dispatchMessage(DispatcherMessage& msg, Client& client);

	Notification& subscribeClientForNotifications(Client& client, SomeIP::MemberIDs messageID) {
		Notification& notification = getOrCreateNotification(messageID);
		notification.subscribe(client);
		return notification;
	}

	std::string dumpState();

	void cleanDisconnectedClients();

	Notification& getOrCreateNotification(SomeIP::MemberIDs messageID) {

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

	Service* tryRegisterService(SomeIP::ServiceIDs serviceID, Client& client, bool isLocal = true);
	ReturnCode registerService(Service& service);

	void unregisterService(Service& service);

	Service* getService(SomeIP::ServiceIDs serviceID) {
		for (auto& service : m_services) {
			if (service->getServiceIDs() == serviceID)
				return service;
		}
		return nullptr;
	}

	void sendPingMessages();

	void addBlackListFilter(const BlackListHostFilter& filter) {
		m_blackList.push_back(&filter);
	}

	const std::vector<const BlackListHostFilter*> getBlackList() {
		return m_blackList;
	}

private:
	vector<Notification*> m_notifications;
	vector<Service*> m_services;
	vector<Client*> m_clients;
	vector<Client*> m_disconnectedClients;
	vector<ServiceRegistrationListener*> m_serviceRegistrationListeners;
	vector<const BlackListHostFilter*> m_blackList;

	int m_messageCounter;

	std::unique_ptr<IdleMainLoopHook> m_idleCallback;
	std::unique_ptr<TimeOutMainLoopHook> m_pingTimer;

	ClientIdentifier m_nextAvailableClientID = 0;

};

//void trace_message(const DispatcherMessage& msg);

}
