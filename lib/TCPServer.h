#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include "SomeIP-common.h"

#include "ipc.h"
#include "Dispatcher.h"
#include "SocketStreamConnection.h"
#include "TCPClient.h"

namespace SomeIP_Dispatcher {

typedef uint16_t TCPPort;

class TCPServer : private BlackListHostFilter, public SocketStreamServer {

	LOG_DECLARE_CLASS_CONTEXT("TCPS", "TCPServer");

public:
	static const TCPPort DEFAULT_TCP_SERVER_PORT = 10032;

	TCPServer(Dispatcher& dispatcher, TCPManager& tcpManager, TCPPort port, MainLoopContext& mainContext) : SocketStreamServer(mainContext),
		m_dispatcher(dispatcher), m_tcpManager(tcpManager), m_port(port), m_mainContext(mainContext) {
	}

	virtual ~TCPServer() {
	}

	bool isBlackListed(const IPv4TCPEndPoint& server, ServiceIDs serviceID) const override;

	static gboolean onNewSocketConnection(GIOChannel* gio, GIOCondition condition, gpointer data) {
		TCPServer* server = static_cast<TCPServer*>(data);
		return server->handleNewConnection();
	}

	bool isServiceRegistered(const Service& service) {
		return (getServiceInstanceNamespace().count(service.getServiceIDs().serviceID) != 0) &&
				(getServiceInstanceNamespace().at(service.getServiceIDs().serviceID)->getServiceIDs().instanceID == service.getServiceIDs().instanceID);
	}

	bool isServiceRegistered(ServiceID serviceID) {
		return (getServiceInstanceNamespace().count(serviceID) != 0);
	}

	void createNewClientConnection(int fileDescriptor) override;

	void onClientDisconnected(const Client& client) {
		delete &client;
	}

	int getLocalTCPPort() {
		return m_port;
	}

	const std::vector<IPv4TCPEndPoint> getIPAddresses() const {
		return m_activePorts;
	}

	static std::vector<IPV4Address> getAllIPAddresses();

	SomeIPReturnCode init(int portCount = 1);

	const ServiceInstanceNamespace& getServiceInstanceNamespace() const {
		return m_instanceNamespace;
	}

	void addService(const Service& service) {
		assert(m_instanceNamespace.count(service.getServiceIDs().serviceID) == 0);
		m_instanceNamespace[service.getServiceIDs().serviceID] = &service;
		log_debug() << m_instanceNamespace;
	}

	void removeService(const Service& service) {
		assert(m_instanceNamespace.count(service.getServiceIDs().serviceID) == 1);
		m_instanceNamespace.erase(service.getServiceIDs().serviceID);
		log_debug() << m_instanceNamespace;
	}

private:
	std::vector<IPv4TCPEndPoint> m_activePorts;
	Dispatcher& m_dispatcher;
	TCPManager& m_tcpManager;
	TCPPort m_port = -1;
	MainLoopContext& m_mainContext;
	ServiceInstanceNamespace m_instanceNamespace;

};

}
