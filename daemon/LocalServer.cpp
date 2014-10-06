#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include "SomeIP-common.h"

#include "LocalClient.h"
#include "LocalServer.h"
#include "TCPServer.h"

namespace SomeIP_Dispatcher {

void LocalServer::init(const char* socketPath) {

#ifdef ENABLE_SYSTEMD
	int systemdPassedSockets = sd_listen_fds(false);

	if (systemdPassedSockets == 1) {
		// we got a file descriptor from systemd => use it instead of creating a new one
		m_fileDescriptor = SD_LISTEN_FDS_START;
		log_info() << "Got a file descriptor from systemd";
	} else
		initServerSocket(socketPath);
#else
	initServerSocket(socketPath);
#endif

	// when running as root, allow the applications which are not running as root to connect
	if ( chmod(getSocketPath(), S_IRWXU | S_IRWXG         // | S_IRWXO
		   ) ) {
		log_warning() << "Cannot set the permission on the socket";
		//			throw ConnectionException("Cannot set the permission on the socket");
	}

	log_info() << "UNIX domain server socket listening on path " << getSocketPath();

}

}
