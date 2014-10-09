#include "TCPManager.h"
#include "TCPClient.h"
#include "TCPServer.h"

namespace SomeIP_Dispatcher {

RemoteTCPClient& TCPManager::getOrCreateClient(const IPv4TCPEndPoint& serverID) {

	// Check whether we already know that server
	RemoteTCPClient* client = nullptr;
	for (auto existingClient : m_clients) {
		if ( serverID == existingClient->getServerID() ) {
			client = existingClient;
		}
	}

	// no matching client found => create a new one
	if (client == nullptr) {
		client = new RemoteTCPClient(m_dispatcher, *this, m_mainLoopContext, serverID);
		m_clients.push_back(client);
		client->registerClient();
	}

	return *client;
}

void TCPManager::onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					  const IPv4ConfigurationOption* address,
					  const SomeIPServiceDiscoveryMessage& message) {

	IPv4TCPEndPoint serverID(address->m_address, address->m_port);

	// ignore our own notifications
	for ( auto filter : m_dispatcher.getBlackList() ) {
		if ( filter->isBlackListed(serverID, ServiceIDs(serviceEntry.m_serviceID, serviceEntry.m_instanceID ) ) ) {
			log_debug() << "Ignoring service " << serverID.toString();
			return;
		}
	}

	// Check whether we already know that server
	RemoteTCPClient& client = getOrCreateClient(serverID);

	bool rebootDetected = client.detectReboot(message, true);

	if (rebootDetected) {
		log_warning() << "Reboot detected from";
		client.onRebootDetected();
	}

	client.onServiceAvailable(ServiceIDs(serviceEntry.m_serviceID, serviceEntry.m_instanceID));

	log_info("Remote service with serviceID:%i, InstanceID:%i is available on IP address: %s port:%i",
		 serviceEntry.m_serviceID, serviceEntry.m_instanceID, address->m_address.toString().c_str(), address->m_port);

}


void TCPManager::onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					    const IPv4ConfigurationOption* address,
					    const SomeIPServiceDiscoveryMessage& message) {

	IPv4TCPEndPoint serverID(address->m_address, address->m_port);

	// ignore our own notifications
	for ( auto filter : m_dispatcher.getBlackList() )
		if ( filter->isBlackListed(serverID) ) {
			log_debug() << "Ignoring service " << serverID.toString();
			return;
		}

	// Check whether we already know that server
	TCPClient& client = getOrCreateClient(serverID);

	bool rebootDetected = client.detectReboot(message, true);

	if (rebootDetected) {
		log_warning() << "Reboot detected from";
		client.onRebootDetected();
	}

	client.onServiceUnavailable(ServiceIDs(serviceEntry.m_serviceID, serviceEntry.m_instanceID));

	log_info("Remote service with serviceID:%i, InstanceID:%i is NOT available on IP address: %s port:%i",
		 serviceEntry.m_serviceID, serviceEntry.m_instanceID, address->m_address.toString().c_str(), address->m_port);

}

}
