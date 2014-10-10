#include "TCPManager.h"
#include "TCPClient.h"
#include "TCPServer.h"

namespace SomeIP_Dispatcher {

RemoteTCPClient* TCPManager::getOrCreateClient(const IPv4TCPEndPoint& serverID) {

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

	return client;
}

void TCPManager::onRemoteServiceAvailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					  const IPv4ConfigurationOption* address,
					  const SomeIPServiceDiscoveryMessage& message) {

	IPv4TCPEndPoint serverID(address->m_address, address->m_port);
	ServiceIDs serviceIDs(serviceEntry.m_serviceID, serviceEntry.m_instanceID);

	// ignore our own notifications
	for ( auto filter : m_dispatcher.getBlackList() ) {
		if ( filter->isBlackListed(serverID, serviceIDs ) ) {
			log_debug() << "Ignoring service " << serverID.toString();
			return;
		}
	}

	// Check whether we already know that server
	RemoteTCPClient* client = getOrCreateClient(serverID);

	if (client->detectReboot(message, true)) {
		log_warning() << "Reboot detected from";
		onClientReboot(*client);
		client = getOrCreateClient(serverID);
	}

	client->onServiceAvailable(serviceIDs);

	log_info() << "Remote service " << serviceIDs.toString()<< " is available on " << serverID.toString();

}


void TCPManager::onRemoteServiceUnavailable(const SomeIPServiceDiscoveryServiceEntry& serviceEntry,
					    const IPv4ConfigurationOption* address,
					    const SomeIPServiceDiscoveryMessage& message) {

	IPv4TCPEndPoint serverID(address->m_address, address->m_port);
	ServiceIDs serviceIDs(serviceEntry.m_serviceID, serviceEntry.m_instanceID);

	// ignore our own notifications
	for ( auto filter : m_dispatcher.getBlackList() )
		if ( filter->isBlackListed(serverID) ) {
			log_debug() << "Ignoring service " << serverID.toString();
			return;
		}

	// Check whether we already know that server
	RemoteTCPClient* client = getOrCreateClient(serverID);

	if (client->detectReboot(message, true)) {
		log_warning() << "Reboot detected from";
		onClientReboot(*client);
	}
	else {
		client->onServiceUnavailable(serviceIDs);
		log_info() << "Remote service " << serviceIDs.toString()<< " is NOT available on " << serverID.toString();
	}


}

}
