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

#include "GlibMainLoopInterfaceImplementation.h"

namespace SomeIP_Dispatcher {

LOG_DECLARE_DEFAULT_CONTEXT(mainContext, "main", "Main log context");

int run(int argc, const char** argv) {

	CommandLineParser commandLineParser("Dispatcher", "", SOMEIP_PACKAGE_VERSION);

	int tcpPortNumber = TCPServer::DEFAULT_TCP_SERVER_PORT;
	commandLineParser.addOption(tcpPortNumber, "port", 'p', "TCP port number");

	int tcpPortTriesCount = 10;
	commandLineParser.addOption(tcpPortTriesCount, "portcount", 'u', "Number of consecutive TCP port to try");

	const char* activationConfigurationFolder = SOMEIP_ACTIVATION_CONFIGURATION_FOLDER;
	commandLineParser.addOption(activationConfigurationFolder, "conf", 'c', "Auto-activation configuration folder");

	const char* logFilePath = "/tmp/someip_dispatcher.log";
	commandLineParser.addOption(logFilePath, "log", 'l', "Log file path");

	bool disableLocalIPC = false;
	commandLineParser.addOption(disableLocalIPC, "ipc", 'i', "Disable local IPC");

	bool disableRemoteServices = false;
	commandLineParser.addOption(disableRemoteServices, "rem", 'r', "Disable remote services registration");

	const char* localSocketPath = SomeIPClient::ClientDaemonConnection::DEFAULT_SERVER_SOCKET_PATH;
	commandLineParser.addOption(localSocketPath, "localPath", 's', "Local IPC socket path");

	if ( commandLineParser.parse(argc, argv) )
		exit(1);

	SomeIPFileLoggingContext::openFile(logFilePath);

	MainLoopApplication app;

	//	auto& mainLoopContext = app.getMainContext();

	GlibMainLoopInterfaceImplementation mainLoopContext( app.getMainContext() );

	Dispatcher dispatcher(mainLoopContext);

	log_info() << "Daemon started. version: " << SOMEIP_PACKAGE_VERSION << ". Logging to : " << logFilePath;

	TCPManager tcpManager(dispatcher, mainLoopContext);

	TCPServer tcpServer(dispatcher, tcpManager, tcpPortNumber, mainLoopContext);
	if(isError(tcpServer.init(tcpPortTriesCount))) {
		return -1;
	}

	for ( auto& localIpAddress : tcpServer.getIPAddresses() )
		log_debug() << "Local IP address : " << localIpAddress.toString();

	LocalServer localServer(dispatcher, mainLoopContext);
	if (!disableLocalIPC)
		localServer.init(localSocketPath);

	ServiceAnnouncer serviceAnnouncer(dispatcher, tcpServer, mainLoopContext);
	serviceAnnouncer.init();

	RemoteServiceListener remoteServiceListener(dispatcher, tcpManager, serviceAnnouncer, mainLoopContext);
	if (!disableRemoteServices)
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
