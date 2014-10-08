#pragma once

#include <string>
#include <string.h>
#include <poll.h>
#include <cstdint>

#include "SomeIP-common.h"
#include "SomeIP.h"

#include "ipc.h"

#include "Message.h"
#include "SomeIP-clientLib.h"

#include "Dispatcher.h"
#include "TCPManager.h"
#include "GlibMainLoopInterfaceImplementation.h"
#include "RemoteServiceListener.h"

namespace SomeIPClient {

using namespace SomeIP_Lib;
using namespace SomeIP_utils;
using namespace SomeIP_Dispatcher;

LOG_IMPORT_CONTEXT(clientLibContext);

/**
 * Interface to be used by client application to connect to the dispatcher. The dispatcher can either be local
 */
class DaemonLessClient : public ClientConnection, private ServiceRegistrationListener {

	LOG_DECLARE_CLASS_CONTEXT("DLES" , "Daemonless connection");

	class DaemonInterface : public Client {

	public:

		DaemonInterface(DaemonLessClient& client, Dispatcher& dispatcher) : Client(dispatcher),
			m_client(client), m_dispatcher(dispatcher) {
		}

		void init() override {
		}

		void onNotificationSubscribed(SomeIP::ServiceIDs serviceID, SomeIP::MemberID memberID) override {
		}

		/**
		 * Returns a string representation of the object
		 */
		std::string toString() const override {
			return std::string();
		}

		void sendMessage(const DispatcherMessage& msg) override {
			m_client.m_listener->processMessage(msg);
		}

		SomeIPReturnCode sendMessage(const OutputMessage& msg) override {
			m_client.m_listener->processMessage(msg);
			return SomeIPReturnCode::OK;
		}

		/**
		 * Returns true if the connection to the daemon is active
		 */
		bool isConnected() const override {
			return m_client.isConnected();
		}

		/**
		 * Sends the given message to the dispatcher and block until a response is received.
		 */
		InputMessage sendMessageBlocking(const OutputMessage& msg) override {
			assert(false);
		}
private:
		DaemonLessClient& m_client;
		Dispatcher& m_dispatcher;

	};

public:
	DaemonLessClient(MainLoopContext& mainLoopContext) : m_dispatcher(mainLoopContext)
, tcpManager(m_dispatcher, mainLoopContext, m_tcpPortNumber)
, serviceAnnouncer(m_dispatcher, tcpManager, mainLoopContext)
, remoteServiceListener(m_dispatcher, tcpManager, serviceAnnouncer, mainLoopContext), m_daemonInterface(*this, m_dispatcher)
{
		setMainLoopInterface(mainLoopContext);
		m_dispatcher.addServiceRegistrationListener(*this);
	}

	virtual ~DaemonLessClient() {
	}

	/**
	 * Handles the data which has been received.
	 * @return : true if there is still some data to be processed
	 */
	bool dispatchIncomingMessages() override {
		assert(false);
	}

	void disconnect() override {
	}


	void onServiceRegistered(const Service& service) override {
		ClientConnection::onServiceRegistered(service.getServiceIDs());
	}

	virtual void onServiceUnregistered(const Service& service) override {
		ClientConnection::onServiceUnregistered(service.getServiceIDs());
	}

	/**
	 * Registers a new service.
	 */
	SomeIPReturnCode registerService(SomeIP::ServiceIDs serviceID) override {
		auto service = m_dispatcher.tryRegisterService(serviceID, m_daemonInterface, true);

		if (service != nullptr) {
			m_registeredServices[serviceID] = service;
			return SomeIPReturnCode::OK;
		}
		else
			return SomeIPReturnCode::ERROR;

	}

	std::unordered_map<ServiceIDs, Service*> m_registeredServices;

	/**
	 * Unregisters the given service
	 */
	SomeIPReturnCode unregisterService(SomeIP::ServiceIDs serviceID) override {
		if (m_registeredServices.count(serviceID) == 0)
			return SomeIPReturnCode::ERROR;

		m_dispatcher.unregisterService(*m_registeredServices[serviceID]);
		m_registeredServices[serviceID] = nullptr;

		return SomeIPReturnCode::OK;
	}

	/**
	 * Subscribes to notifications for the given MessageID
	 */
	 SomeIPReturnCode subscribeToNotifications(SomeIP::MemberIDs messageID) override {
		 m_dispatcher.subscribeClientForNotifications(m_daemonInterface, messageID);
		 return SomeIPReturnCode::OK;
	 }

	/**
	 * Sends the given message to the dispatcher.
	 */
	 SomeIPReturnCode sendMessage(const OutputMessage& msg) override {
		 InputMessage inputMessage(msg);
		 m_dispatcher.dispatchMessage(inputMessage, m_daemonInterface);
		 return SomeIPReturnCode::OK;
	 }

	/**
	 * Sends a ping message to the connected peers
	 */
	 SomeIPReturnCode sendPing() override {
		 m_dispatcher.sendPingMessages();
		 return SomeIPReturnCode::OK;
	 }

	/**
	 * Sends the given message to the dispatcher and block until a response is received.
	 */
	 InputMessage sendMessageBlocking(const OutputMessage& msg) override {
		 assert(msg.getHeader().isRequestWithReturn());
		 Service* service = m_dispatcher.getService(ServiceIDs(msg.getHeader().getServiceID(), msg.getInstanceID()));
		 Client* client = service->getClient();
		 assert(client != nullptr);
		 return client->sendMessageBlocking(msg);
	 }

	/**
	 * Connects to the dispatcher.
	 */
	SomeIPReturnCode connect(ClientConnectionListener& clientReceiveCb) override {

		auto code = tcpManager.init(tcpPortTriesCount);
		if (isError(code))
			return code;

		for (auto& localIpAddress : tcpManager.getIPAddresses())
			log_debug() << "Local IP address : " << localIpAddress.toString();

		code = serviceAnnouncer.init();
		if (isError(code))
			return code;

		code = remoteServiceListener.init();
		if (isError(code))
			return code;

		m_listener = &clientReceiveCb;

		m_connected = true;

		return SomeIPReturnCode::OK;
	}


	/**
	 * Returns true if the connection to the daemon is active
	 */
	bool isConnected() const override {
		return m_connected;
	}

	/**
	 * Returns true if some data has been received
	 */
	 bool hasIncomingMessages() override {
		assert(false);
		return false;
	 }

	bool isServiceAvailableBlocking(ServiceIDs service) override {
		return getServiceRegistry().isServiceRegistered(service);
	}

private:
	int m_tcpPortNumber = 6666;
	int tcpPortTriesCount = 10;

	Dispatcher m_dispatcher;
	TCPManager tcpManager;
	ServiceAnnouncer serviceAnnouncer;
	RemoteServiceListener remoteServiceListener;

	DaemonInterface m_daemonInterface;
	ClientConnectionListener* m_listener = nullptr;

	bool m_connected = false;

};

}
