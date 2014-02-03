#include "TCPManager.h"
#include "TCPClient.h"
#include "TCPServer.h"

TCPClient& TCPManager::getOrCreateClient(const IPv4TCPServerIdentifier& serverID) {

	// Check whether we already know that server
	TCPClient* client = NULL;
	for (auto existingClient : m_clients) {
		if ( serverID == existingClient->getServerID() ) {
			client = existingClient;
		}
	}

	// no matching client found => create a new one
	if (client == NULL) {
		client = new TCPClient(m_dispatcher, serverID, *this);
		m_clients.push_back(client);
		client->registerClient();
	}

	return *client;
}

void TCPManager::onRemoteServiceAvailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					  const SomeIP::IPv4ConfigurationOption* address,
					  const SomeIP::SomeIPServiceDiscoveryMessage& message) {

	IPv4TCPServerIdentifier serverID(address->m_address, address->m_port);

	// ignore our own notifications
	for ( auto localAddress : TCPServer::getIPAddresses() )
		if (serverID.m_address == localAddress)
			return;

	// Check whether we already know that server
	TCPClient& client = getOrCreateClient(serverID);

	bool rebootDetected = client.detectReboot(message, true);

	if (rebootDetected) {
		log_warn("Reboot detected from");
		client.onRebootDetected();
	}

	client.onServiceAvailable(serviceEntry.m_serviceID);

	log_info("New remote service with serviceID:%i, InstanceID:%i is available on IP address: %s port:%i",
		 serviceEntry.m_serviceID, serviceEntry.m_instanceID, address->m_address.toString().c_str(), address->m_port);

}

void TCPManager::onRemoteServiceUnavailable(const SomeIP::SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					    const SomeIP::IPv4ConfigurationOption* address,
					    const SomeIP::SomeIPServiceDiscoveryMessage& message) {

	IPv4TCPServerIdentifier serverID(address->m_address, address->m_port);

	// ignore our own notifications
	for ( auto localAddress : TCPServer::getIPAddresses() )
		if (serverID.m_address == localAddress)
			return;

	// Check whether we already know that server
	TCPClient& client = getOrCreateClient(serverID);

	bool rebootDetected = client.detectReboot(message, true);

	if (rebootDetected) {
		log_warn("Reboot detected from");
		client.onRebootDetected();
	}

	client.onServiceUnavailable(serviceEntry.m_serviceID);

	log_info("New remote service with serviceID:%i, InstanceID:%i is NOT available on IP address: %s port:%i",
		 serviceEntry.m_serviceID, serviceEntry.m_instanceID, address->m_address.toString().c_str(), address->m_port);

}