#include "SomeIP-common.h"

LOG_DEFINE_APP_IDS("Some", "SomeIP daemon");

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

#include "SomeIP-clientLib.h"

namespace SomeIP_Dispatcher {

LOG_DECLARE_DEFAULT_CONTEXT(mainContext, "main", "Main log context");

int run(int argc, const char** argv) {

	CommandLineParser commandLineParser("Dispatcher", "", SOMEIP_PACKAGE_VERSION);
	int tcpPortNumber = TCPServer::DEFAULT_TCP_SERVER_PORT;
	int tcpPortTriesCount = 10;
	bool disableLocalIPC = false;
	const char* activationConfigurationFolder = SOMEIP_ACTIVATION_CONFIGURATION_FOLDER;
	const char* logFilePath = "/tmp/someip_dispatcher.log";
	const char* localSocketPath = SomeIPClient::ClientDaemonConnection::DEFAULT_SERVER_SOCKET_PATH;

	commandLineParser.addOption(tcpPortNumber, "port", 'p', "TCP port number");
	commandLineParser.addOption(tcpPortTriesCount, "portcount", 'u', "Number of consecutive TCP port to try");
	commandLineParser.addOption(activationConfigurationFolder, "conf", 'c', "Auto-activation configuration folder");
	commandLineParser.addOption(logFilePath, "log", 'l', "Log file path");
	commandLineParser.addOption(disableLocalIPC, "ipc", 'i', "Disable local IPC");
	commandLineParser.addOption(localSocketPath, "localPath", 's', "Local IPC socket path");

	if ( commandLineParser.parse(argc, argv) )
		exit(1);

	SomeIPFileLoggingContext::openFile(logFilePath);

	MainLoopApplication app;

	Dispatcher dispatcher;

	log_info() << "Daemon started. version: " << SOMEIP_PACKAGE_VERSION << ". Logging to : " << logFilePath;

	TCPManager tcpManager(dispatcher);

	TCPServer tcpServer(dispatcher, tcpManager, tcpPortNumber);
	tcpServer.init(tcpPortTriesCount);

	for ( auto& localIpAddress : tcpServer.getIPAddresses() )
		log_debug() << "Local IP address : " << localIpAddress.toString();

	LocalServer localServer(dispatcher);
	if (!disableLocalIPC)
		localServer.init(localSocketPath);

	ServiceAnnouncer serviceAnnouncer(dispatcher, tcpServer);
	serviceAnnouncer.init();

	RemoteServiceListener remoteServiceListener(dispatcher, tcpManager, serviceAnnouncer);
	remoteServiceListener.init();

	WellKnownServiceManager wellKnownServiceManager(dispatcher);
	wellKnownServiceManager.init(activationConfigurationFolder);

	app.run();

	log_info() << "Shutting down...";

	return 0;

}
}


int main(int argc, const char** argv) {
	SomeIP_Dispatcher::run(argc, argv);
}
