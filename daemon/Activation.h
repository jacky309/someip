#pragma once

#include <dirent.h>
#include <unistd.h>

#include "Dispatcher.h"
#include "LocalClient.h"
#include "SomeIP.h"

#include "glib.h"

#ifdef ENABLE_SYSTEMD
#include "dbus/dbus-glib.h"
#endif

namespace SomeIP_Dispatcher {

class WellKnownServiceManager;

LOG_IMPORT_CONTEXT(activationContext);

#ifdef ENABLE_SYSTEMD

/**
 * This class is used to activate a service using SystemD's DBUS interface
 */
class SystemDActivator {

	LOG_SET_CLASS_CONTEXT(activationContext);

	static constexpr const char* SYSTEMD_DBUS_SERVICE_NAME = "org.freedesktop.systemd1";
	static constexpr const char* SYSTEMD_DBUS_INTERFACE_NAME = "org.freedesktop.systemd1.Unit";

public:
	bool activateSystemDService(const char* serviceName);
	void init();

private:
	DBusGConnection* connection = nullptr;

};
#endif

class WellKnownService : public Service {

	LOG_SET_CLASS_CONTEXT(activationContext);

	enum class ProcessState {
		STOPPED, STARTING, RUNNING, START_FAILED
	};

public:
	WellKnownService(WellKnownServiceManager& serviceManager, SomeIP::ServiceID serviceID, const char* commandLine,
			 const char* workingDirectory,
			 const char* systemDServiceName) :
		Service(serviceID, true), m_serviceManager(serviceManager), m_commandLine(commandLine), m_workingDirectory(
			workingDirectory), m_systemDServiceName(systemDServiceName) {
	}

	LocalClient* getLocalClient() {
		return static_cast<LocalClient*>( getClient() );
	}

	ReturnCode setClient(Client& client) override;

	void sendMessage(DispatcherMessage& msg) override;

	SomeIPReturnCode activateService();

	std::string toString() const override {
		if (m_client != nullptr) {
			return Service::toString();
		} else {
			char buffer[2000];
			snprintf( buffer, sizeof(buffer), "Service ServiceID: 0x%X, Activatable service at %s", getServiceID(),
				  m_commandLine.c_str() );
			return buffer;
		}
	}

private:
	WellKnownServiceManager& m_serviceManager;

	std::string m_commandLine;
	std::string m_workingDirectory;
	std::string m_systemDServiceName;

	ProcessState m_state = ProcessState::STOPPED;
	vector<IPCMessage> m_pendingMessages;

};

class WellKnownServiceManager : private BlackListHostFilter {

	static constexpr const char* SERVICE_SECTION = "Service";
	static constexpr const char* SERVICE_FILENAME_EXTENSION = "someip-service";

	LOG_SET_CLASS_CONTEXT(activationContext);

public:
	WellKnownServiceManager(Dispatcher& dispatcher) :
		m_dispatcher(dispatcher) {
	}

	~WellKnownServiceManager() {
		for(auto service : m_services) {
			m_dispatcher.unregisterService(*service);
			delete service;
		}
	}

	Dispatcher& getDispatcher() {
		return m_dispatcher;
	}

	void init(const char* configurationFolder);

	void readConfiguration(const char* folder);

	bool isBlackListed(const IPv4TCPEndPoint& server, ServiceID serviceID) const override;

#ifdef ENABLE_SYSTEMD
	SystemDActivator& getSystemDActivator() {
		return m_activator;
	}
private:
	SystemDActivator m_activator;
#endif

private:
	Dispatcher& m_dispatcher;
	std::vector<WellKnownService*> m_services;

};


}
