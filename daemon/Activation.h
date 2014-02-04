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

class WellKnownService : public Service {

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

	ReturnCode setClient(Client& client) override {
		auto v = Service::setClient(client);
		if (v == ReturnCode::OK) {

			// Send the pending messages
			for (auto& msg : m_pendingMessages)
				getLocalClient()->sendIPCMessage(msg);
			m_pendingMessages.resize(0);

		}
		return v;
	}

	void sendMessage(DispatcherMessage& msg) override;

	std::string toString() const override {
		if (m_client != NULL) {
			return Service::toString();
		} else {
			char buffer[2000];
			snprintf( buffer, sizeof(buffer), "Service ServiceID: 0x%X, Activatable service at %s", getServiceID(),
				  m_commandLine.c_str() );
			return buffer;
		}
	}

	enum class ProcessState {
		STOPPED, STARTING, RUNNING, START_FAILED
	};

	bool activateSystemDService();

	SomeIPFunctionReturnCode activateService() {

		log_info( "Activating service %s", toString().c_str() );

		if (m_state != ProcessState::STARTING) {

			bool bStarted = false;

			if (m_systemDServiceName.size() != 0) {
				bStarted = activateSystemDService();
				m_state = ProcessState::STARTING;
			}

			if (!bStarted) {

				// we did not start via systemD, start as command line

				if (chdir( m_workingDirectory.c_str() ) != 0) {
					log_debug( "Can't change current directory to %s", m_workingDirectory.c_str() );
					throw new Exception("Bad configuration");
				}

				GError* error = NULL;
				if ( g_spawn_command_line_async(m_commandLine.c_str(), &error) ) {
					m_state = ProcessState::STARTING;
				} else {
					log_error( "Can't start application %s", m_commandLine.c_str() );
					m_state = ProcessState::START_FAILED;
				}

			}
		}

		return (m_state != ProcessState::START_FAILED) ? SomeIPFunctionReturnCode::OK : SomeIPFunctionReturnCode::ERROR;

	}

	WellKnownServiceManager& m_serviceManager;

	std::string m_commandLine;
	std::string m_workingDirectory;
	std::string m_systemDServiceName;

	ProcessState m_state = ProcessState::STOPPED;
	vector<IPCMessage> m_pendingMessages;

};

class WellKnownServiceManager {

	static constexpr const char* SERVICE_SECTION = "Service";
	static constexpr const char* SERVICE_FILENAME_EXTENSION = "someip-service";

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

	void init(const char* configurationFolder) {

		readConfiguration(configurationFolder);

#ifdef ENABLE_SYSTEMD

		GError* error = NULL;

		connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
		if (connection == NULL) {
			log_error("Failed to open connection to dbus: %s", error->message);
			throw Exception("Failed to open connection to dbus");
		}

#endif

	}


#ifdef ENABLE_SYSTEMD

	static constexpr const char* SYSTEMD_DBUS_SERVICE_NAME = "org.freedesktop.systemd1";
	static constexpr const char* SYSTEMD_DBUS_INTERFACE_NAME = "org.freedesktop.systemd1.Unit";

	static std::string getSystemDPath(const char* serviceName) {
		std::string s = "/org/freedesktop/systemd1/unit/";
		s += serviceName;
		s += "_2eservice";
		return s;
	}

	bool activateSystemDService(const char* serviceName) {

		std::string path = getSystemDPath(serviceName);

		log_debug( "Path = %s", path.c_str() );

		/* Create a proxy object for the "bus driver" (name "org.freedesktop.DBus") */
		DBusGProxy* proxy = dbus_g_proxy_new_for_name(connection, SYSTEMD_DBUS_SERVICE_NAME, path.c_str(),
							      SYSTEMD_DBUS_INTERFACE_NAME);

		const char* returnValue = NULL;

		GError* error = NULL;
		if ( !dbus_g_proxy_call(proxy, "Start", &error, G_TYPE_STRING, "replace", G_TYPE_INVALID,
					G_TYPE_STRING, &returnValue, G_TYPE_INVALID) ) {

			//			if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
			//				log_error("Caught remote method exception %s: %s", dbus_g_error_get_name(error), error->message);
			//			} else {
			//				log_error("Error: %s", error->message);
			//			}
			//
			//			g_error_free(error);
			//			exit(1);
		}

		log_info("Return value : %s", (returnValue == NULL) ? "null" : returnValue);

		g_object_unref(proxy);

		return true;

		/* Print the results */
		//		g_print("Names on the message bus:\n");
		//		for (name_list_ptr = name_list; *name_list_ptr; name_list_ptr++) {
		//			g_print("  %s\n", *name_list_ptr);
		//		}
		//		g_strfreev(name_list);

	}
#endif

	void readConfiguration(const char* folder) {

		DIR* dir = opendir(folder);
		if (dir != NULL) {
			struct dirent* dp;
			while ( ( dp = readdir(dir) ) != NULL ) {

				if ( strstr(dp->d_name, SERVICE_FILENAME_EXTENSION)
				     == dp->d_name + strlen(dp->d_name) - strlen(SERVICE_FILENAME_EXTENSION) ) {

					GString* str = g_string_new(folder);
					g_string_append(str, "/");
					g_string_append(str, dp->d_name);

					GKeyFile* gpMyKeyFile = g_key_file_new();
					GKeyFileFlags myConfFlags =
						static_cast<GKeyFileFlags>(G_KEY_FILE_NONE | G_KEY_FILE_KEEP_COMMENTS
									   | G_KEY_FILE_KEEP_TRANSLATIONS);

					const char* fileName = str->str;

					GError* pGError = NULL;
					if ( !g_key_file_load_from_file(gpMyKeyFile, fileName, myConfFlags, &pGError) ) {
						g_key_file_free(gpMyKeyFile);
						gpMyKeyFile = NULL;
						g_clear_error(&pGError);
					}

					gchar* commandLine = g_key_file_get_string(gpMyKeyFile, SERVICE_SECTION, "ExecStart",
										   &pGError);
					gchar* workingDirectory =
						g_key_file_get_string(gpMyKeyFile, SERVICE_SECTION, "WorkingDirectory",
								      &pGError);
					gchar* systemDServiceName =
						g_key_file_get_string(gpMyKeyFile, SERVICE_SECTION, "SystemdService",
								      &pGError);
					SomeIP::ServiceID serviceID =
						g_key_file_get_integer(gpMyKeyFile, SERVICE_SECTION, "ServiceID",
								       &pGError);

					WellKnownService* testService = new WellKnownService(
						*this, serviceID, commandLine, workingDirectory,
						(systemDServiceName != nullptr) ? systemDServiceName : "");

					//					if (commandLine)
					free(commandLine);
					//					if (workingDirectory)
					free(workingDirectory);
					//					if (systemDServiceName)
					free(systemDServiceName);

					m_dispatcher.registerService(*testService);
					m_services.push_back(testService);

					g_key_file_unref(gpMyKeyFile);

					g_string_free(str, true);
				}
			}

			closedir(dir);
		} else {
			log_warning("Can't open directory %s", folder);
		}
	}

private:
	Dispatcher& m_dispatcher;
	std::vector<WellKnownService*> m_services;

#ifdef ENABLE_SYSTEMD
	DBusGConnection* connection;
#endif

};

#ifdef ENABLE_SYSTEMD
bool WellKnownService::activateSystemDService() {
	return m_serviceManager.activateSystemDService( m_systemDServiceName.c_str() );
}
#else
bool WellKnownService::activateSystemDService() {
	return false;
}
#endif


void WellKnownService::sendMessage(DispatcherMessage& msg) {
	if (getLocalClient() == NULL) {
		if (activateService() == SomeIPFunctionReturnCode::OK) {
			m_pendingMessages.push_back( msg.getIPCMessage() );
			log_debug( "Pushed message %s", m_pendingMessages[m_pendingMessages.size() - 1].toString().c_str() );
		} else {
			log_error() << "Can't start " << toString();
			OutputMessage responseMessage = createMethodReturn(msg);
			responseMessage.getHeader().setMessageType(SomeIP::MessageType::ERROR);
			auto client = m_serviceManager.getDispatcher().getClientFromId( msg.getClientIdentifier() );
			client->sendMessage(responseMessage);
		}

	} else
		getLocalClient()->sendMessage(msg);
}

}
