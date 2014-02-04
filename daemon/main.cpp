#include "SomeIP-common.h"

LOG_DEFINE_APP_IDS("Some", "SomeIP daemon");
LOG_DECLARE_DEFAULT_CONTEXT(mainContext, "main", "Main log context");

#include "CommandLineParser.h"
#include "config.h"
#include "glib.h"
#include "Client.h"
#include "Dispatcher.h"
#include "LocalServer.h"
#include "TCPServer.h"
#include "TCPManager.h"

#include "RemoteServiceListener.h"
#include "ServiceAnnouncer.h"

#include "MainLoopApplication.h"

#include "Activation.h"

using namespace SomeIP;

namespace SomeIP_Dispatcher {

using namespace SomeIP_utils;

class DaemonApplication : public MainLoopApplication {
public:
	DaemonApplication(const DaemonConfiguration& configuration) :
		m_dispatcher(configuration), m_configuration(configuration) {
	}

	Dispatcher& getDispatcher() {
		return m_dispatcher;
	}

	Dispatcher m_dispatcher;
	const DaemonConfiguration& m_configuration;

};

static DaemonConfiguration configuration;

const DaemonConfiguration& getConfiguration() {
	return configuration;
}

}

int main(int argc, const char** argv) {

	using namespace SomeIP_Dispatcher;

	CommandLineParser commandLineParser("Dispatcher", "", SOMEIP_PACKAGE_VERSION);
	int tcpPortNumber = configuration.getDefaultLocalTCPPort();
	const char* activationConfigurationFolder = SOMEIP_ACTIVATION_CONFIGURATION_FOLDER;
	const char* logFilePath = "/dev/stdout";

	commandLineParser.addArgument(tcpPortNumber, "port", 'p', "TCP port number");
	commandLineParser.addArgument(activationConfigurationFolder, "conf", 'c', "Auto-activation configuration folder");
	commandLineParser.addArgument(logFilePath, "log", 'l', "Log file path");

	if ( commandLineParser.parse(argc, argv) ) {
	exit(1);
	}

#ifdef ENABLE_CONSOLE_LOGGING
	ConsoleLogOutput logOuput(logFilePath);
	ConsoleLogOutput::setInstance(logOuput);
#endif

	configuration.setDefaultLocalTCPPort(tcpPortNumber);

	DaemonApplication app(configuration);

	log_info("Daemon started. version: ") << SOMEIP_PACKAGE_VERSION << ". Logging to : " << logFilePath;

	for ( auto& localIpAddress : TCPServer::getIPAddresses() ) {
		log_debug( "Local IP address : %s", localIpAddress.toString().c_str() );
	}

	TCPManager tcpManager( app.getDispatcher() );

	TCPServer tcpServer(app.getDispatcher(), tcpManager);
	tcpServer.init();

	LocalServer localServer( app.getDispatcher() );
	localServer.init();

	RemoteServiceListener remoteServiceListener(app.getDispatcher(), tcpManager);
	remoteServiceListener.init();

	ServiceAnnouncer serviceAnnouncer(app.getDispatcher(), tcpServer);
	serviceAnnouncer.init();

	WellKnownServiceManager wellKnownServiceManager( app.getDispatcher() );
	wellKnownServiceManager.init(activationConfigurationFolder);

	app.run();

	log_warning("Shutting down...");

	usleep(10000);

	return 0;

}
