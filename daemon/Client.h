#pragma once

#include "Message.h"
#include "Dispatcher.h"
#include <stdlib.h>
#include <stdint.h>
#include "assert.h"
#include <string>
#include <vector>

#include "SomeIP-common.h"

namespace SomeIP_Dispatcher {

class Dispatcher;
class Notification;
class Service;

/**
 * Interface used to be informed about registration/un-registration of local and remote services
 */
class ServiceRegistrationListener {
public:
	virtual ~ServiceRegistrationListener() {
	}
	virtual void onServiceRegistered(const Service& service) = 0;
	virtual void onServiceUnregistered(const Service& service) = 0;
};

/**
 * Abstract client class
 */
class Client : private MessageSource {

	LOG_DECLARE_CLASS_CONTEXT("Clie", "Client");

public:
	Client(Dispatcher& dispatcher) :
		m_dispatcher(dispatcher), m_endPoint(*this) {
	}

	void assignUniqueID() {
		m_id = s_nextAvailableID++;
	}

	virtual ~Client() {
	}

	void unregisterClient();
	void registerClient();

	virtual void init() = 0;

	Dispatcher& getDispatcher() {
		return m_dispatcher;
	}

	/**
	 * Returns a string representation of the object
	 */
	virtual std::string toString() const = 0;

	virtual void sendMessage(const DispatcherMessage& msg) = 0;

	virtual SomeIPFunctionReturnCode sendMessage(OutputMessage& msg) = 0;

	void processIncomingMessage(InputMessage& msg);

	virtual bool isConnected() const = 0;

	/**
	 * Called when a client has subscribed for notifications on the given property
	 */
	virtual void onNotificationSubscribed(SomeIP::MemberID serviceID, SomeIP::MemberID memberID) = 0;

	ClientIdentifier getIdentifier() const {
		return m_id;
	}

	void sendPingMessage() {
		m_pingSender.sendPingMessage([&](OutputMessage & msg) {
						     sendMessage(msg);
					     });
	}

	Service* registerService(SomeIP::ServiceID serviceID, bool isLocal);

	void unregisterService(SomeIP::ServiceID serviceID);

protected:
	void subscribeToNotification(SomeIP::MessageID messageID);

	bool isInputBlocked() {
		return inputBlocked;
	}

	std::vector<Service*> m_registeredServices;

private:
	Dispatcher& m_dispatcher;
	SomeIPEndPoint m_endPoint;
	PingSender m_pingSender;

	/** Indicates whether the client's input has been blocked because a client to which it sent data to is congested */
	bool inputBlocked = false;

	/** Indicates whether the client can't receive anymore data because its reception buffer is full */
	bool isCongested = false;

	std::vector<Notification*> m_subscribedNotifications;

	ClientIdentifier m_id = UNKNOWN_CLIENT;

	/// Counter of messages sent to that application
	unsigned int receivedMessagesCount = 0;

	/// Counter of bytes sent to that application
	unsigned int receivedBytesCount = 0;

	static uint16_t s_nextAvailableID;

	bool m_registered = false;

};

}
