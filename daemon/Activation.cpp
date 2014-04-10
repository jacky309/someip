#include "Activation.h"

namespace SomeIP_Dispatcher {

LOG_DECLARE_CONTEXT(activationContext, "Acti", "Service activation");

ReturnCode WellKnownService::setClient(Client& client) {
	auto v = Service::setClient(client);
	if (v == ReturnCode::OK) {

		// Send the pending messages
		for (auto& msg : m_pendingMessages)
			getLocalClient()->sendIPCMessage(msg);
		m_pendingMessages.resize(0);

	}
	return v;
}


void WellKnownService::sendMessage(DispatcherMessage& msg) {
	if (getLocalClient() == nullptr) {
		if (activateService() == SomeIPReturnCode::OK) {
			m_pendingMessages.push_back( msg.getIPCMessage() );
			log_debug() << "Pushed message" << m_pendingMessages[m_pendingMessages.size() - 1].toString();
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


SomeIPReturnCode WellKnownService::activateService() {

	log_info("Activating service ") << toString();

	if (m_state != ProcessState::STARTING) {

		bool bStarted = false;

#ifdef ENABLE_SYSTEMD
		if (m_systemDServiceName.size() != 0) {
			bStarted = m_serviceManager.getSystemDActivator().activateSystemDService( m_systemDServiceName.c_str() );
			m_state = ProcessState::STARTING;
		}
#endif

		if (!bStarted) {

			// we did not start via systemD, start as command line

			if (chdir( m_workingDirectory.c_str() ) != 0) {
				log_debug() << "Can't change current directory to " << m_workingDirectory;
				throw new Exception("Bad configuration");
			}

			GError* error = nullptr;
			if ( g_spawn_command_line_async(m_commandLine.c_str(), &error) ) {
				m_state = ProcessState::STARTING;
			} else {
				log_error() << "Can't start application : " << m_commandLine;
				m_state = ProcessState::START_FAILED;
			}

		}
	}

	return (m_state != ProcessState::START_FAILED) ? SomeIPReturnCode::OK : SomeIPReturnCode::ERROR;
}

void WellKnownServiceManager::init(const char* configurationFolder) {

	readConfiguration(configurationFolder);

	getDispatcher().addBlackListFilter(*this);

#ifdef ENABLE_SYSTEMD
	m_activator.init();
#endif

}


bool WellKnownServiceManager::isBlackListed(const IPv4TCPServerIdentifier& server, ServiceID serviceID) const {

	// ignore our own services
	for ( auto service : m_services )
		if ( service->getServiceID() == serviceID ) {
			log_debug() << "Local well knownservice with this ID" << serviceID;
			return true;
		}

	return false;
}

void WellKnownServiceManager::readConfiguration(const char* folder) {

	log_info() << "Reading configuration from " << folder;

	DIR* dir = opendir(folder);
	if (dir != nullptr) {
		struct dirent* dp;
		while ( ( dp = readdir(dir) ) != nullptr ) {

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

				GError* pGError = nullptr;
				if ( !g_key_file_load_from_file(gpMyKeyFile, fileName, myConfFlags, &pGError) ) {
					g_key_file_free(gpMyKeyFile);
					gpMyKeyFile = nullptr;
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
		log_warning() << "Can't read from directory " << folder;
	}

}

std::string getSystemDPath(const char* serviceName) {
	std::string s = "/org/freedesktop/systemd1/unit/";
	s += serviceName;
	s += "_2eservice";
	return s;
}

#ifdef ENABLE_SYSTEMD

void SystemDActivator::init() {

	GError* error = nullptr;

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (connection == nullptr) {
		log_error() << "Failed to open connection to dbus: " << error->message;
		throw Exception("Failed to open connection to dbus");
	}

}

bool SystemDActivator::activateSystemDService(const char* serviceName) {

	std::string path = getSystemDPath(serviceName);

	log_debug() << "Path : " << path;

	DBusGProxy* proxy = dbus_g_proxy_new_for_name(connection, SYSTEMD_DBUS_SERVICE_NAME, path.c_str(),
						      SYSTEMD_DBUS_INTERFACE_NAME);

	const char* returnValue = nullptr;

	GError* error = nullptr;
	if ( !dbus_g_proxy_call(proxy, "Start", &error, G_TYPE_STRING, "replace", G_TYPE_INVALID,
				G_TYPE_STRING, &returnValue, G_TYPE_INVALID) ) {
		log_error() << "Could not activate service : " << serviceName;
	}

	log_info() << "Return value : " << ( (returnValue == nullptr) ? "null" : returnValue );

	g_object_unref(proxy);

	return true;
}

#endif


}
