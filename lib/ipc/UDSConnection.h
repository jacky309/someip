#pragma once

#include <functional>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <dirent.h>
#include <unistd.h>

#include "ipc.h"

#include "SocketStreamConnection.h"

namespace SomeIP_Lib {

static constexpr const char* DEFAULT_SERVER_SOCKET_PATH = "/tmp/someIPSocket";
static constexpr const char* ALTERNATIVE_SERVER_SOCKET_PATH = "/tmp/someIPSocket2";

/**
 * Handles a stream connection via Unix domain socket
 */
class UDSConnection : public SocketStreamConnection {

	LOG_DECLARE_CLASS_CONTEXT("UDSC", "UDSConnection");

public:
	IPCOperationReport writeBlocking(const IPCMessage& msg);

	IPCOperationReport writeNonBlocking(const IPCMessage& msg);

	IPCOperationReport readBlocking(IPCInputMessage& msg);

	IPCOperationReport readNonBlocking(IPCInputMessage& msg);

protected:
	typedef std::function<bool (IPCInputMessage&)> IPCMessageReceivedCallbackFunction;

	UDSConnection() {
		uds_socket_path = DEFAULT_SERVER_SOCKET_PATH;
	}

	virtual ~UDSConnection() {
	}

	void connectToServer() {

#ifdef ENABLE_TCP_SOCKET

		char* serverAddress = getenv(SERVER_ADDRESS_ENV_VARIABLE);
		if (serverAddress) {
			{
				struct hostent* ptrh; /* pointer to a host table entry       */
				struct protoent* ptrp; /* pointer to a protocol table entry   */
				struct sockaddr_in sad; /* structure to hold an IP address     */
				int port; /* protocol port number                */
				char* host; /* pointer to host name                */

				memset( (char*) &sad, 0, sizeof(sad) ); /* clear sockaddr structure */
				sad.sin_family = AF_INET; /* set family to Internet     */
				port = TCP_SOCK_STREAM_PORT; /* use default port number      */
				sad.sin_port = htons( (u_short) port );
				host = serverAddress;

				/* Convert host name to equivalent IP address and copy to sad. */
				ptrh = gethostbyname(host);
				if ( ( (char*) ptrh ) == NULL ) {
					log_error("IPC: Failed to connect to server. Invalid address : %s", host);
					throw ConnectionException("");
				}
				memcpy(&(sad.sin_addr), ptrh->h_addr, ptrh->h_length);

				/* Map TCP transport protocol name to protocol number. */
				if ( ( ptrp = getprotobyname("tcp") ) == 0 ) {
					log_error("IPC: Failed to connect to server. Cannot map \"tcp\" to protocol number");
					throw ConnectionException("");
				}

				/* Create a socket. */
				connectionFileDescriptor = socket(AF_INET, SOCK_STREAM, ptrp->p_proto);
				if (connectionFileDescriptor < 0) {
					log_error( "IPC: Failed to connect to server. Socket creation failed. Error : %s",
						   strerror(
							   errno) );
					throw ConnectionException("");
				}

				/* Connect the socket to the specified server. */
				if (connect( connectionFileDescriptor, (struct sockaddr*) &sad, sizeof(sad) ) < 0) {
					log_error( "IPC: Failed to connect to server : %s. Error : %s", serverAddress,
						   strerror(errno) );
					throw ConnectionException("");
				}

				goto connected;
			}
		}

#endif

		int fileDescriptor = socket(AF_UNIX, SOCK_STREAM, 0);

		if ( fileDescriptor == -1 ) {
			log_warning("Failed to create socket");
			throw ConnectionException("Failed to create socket");
		}

		struct sockaddr_un remote;

		remote.sun_family = AF_UNIX;
		strcpy(remote.sun_path, uds_socket_path);

		if (::connect( fileDescriptor, (struct sockaddr*) &remote, strlen(remote.sun_path) +
			       sizeof(remote.sun_family) )
		    == -1) {
			log_warning("Failed to connect to the daemon via socket %s. Trying alternative socket", uds_socket_path);
			uds_socket_path = ALTERNATIVE_SERVER_SOCKET_PATH;
			strcpy(remote.sun_path, uds_socket_path);
			if (::connect( fileDescriptor, (struct sockaddr*) &remote,
				       strlen(remote.sun_path) + sizeof(remote.sun_family) ) == -1) {
				log_error("Failed to connect to daemon via socket %s. ", uds_socket_path);
				throw ConnectionException("Failed to connect to daemon");
			}
		}

		setFileDescriptor(fileDescriptor);
	}

	virtual void handleIncomingIPCMessage(IPCInputMessage& inputMessage) = 0;

	void setInputMessage(IPCInputMessage& msg) {
		m_currentInputMessage = &msg;
		msg.clear();
		m_messageLengthReader.setBuffer( &msg.m_totalMessageSize, sizeof(msg.m_totalMessageSize) );
	}

private:
	IPCOperationReport read(IPCInputMessage& msg, bool blocking);

private:
	const char* uds_socket_path;
	IPCBufferReader m_messageLengthReader;

protected:
	IPCInputMessage* m_currentInputMessage = nullptr;

};


class UDSServer : public SocketStreamServer {

	LOG_DECLARE_CLASS_CONTEXT("UDSServer", "UDSServer");

public:
	UDSServer() {
	}

	void initServerSocket(int fd = 0) {

		m_fileDescriptor = fd;

		if (getFileDescriptor() == 0) {

			if ( ( m_fileDescriptor = socket(AF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
				log_error("Failed to open socket %s", uds_socket_path);
				throw ConnectionException("Failed to open socket");
			}

			struct sockaddr_un local = {.sun_family = AF_UNIX};

			strcpy(local.sun_path, uds_socket_path);
			unlink(local.sun_path);

			if (::bind( getFileDescriptor(), (struct sockaddr*) &local,
				    strlen(local.sun_path) + sizeof(local.sun_family) ) != 0) {
				log_warning("Failed to bind server socket %s. Trying alternate socket.",
					    uds_socket_path);

				uds_socket_path = ALTERNATIVE_SERVER_SOCKET_PATH;
				strcpy(local.sun_path, uds_socket_path);
				unlink(local.sun_path);
				if (::bind( getFileDescriptor(), (struct sockaddr*) &local,
					    strlen(local.sun_path) + sizeof(local.sun_family) ) != 0) {
					log_warning("Failed to bind server socket %s", uds_socket_path);
					throw ConnectionException("Failed to open socket");
				}
			}

			if (listen(getFileDescriptor(), SOMAXCONN) != 0) {
				log_error("Failed to listen to socket %s",
					  uds_socket_path);
				throw ConnectionException("Failed to listen");
			}

		}
	}

	void setSocketPath(const char* path) {
		uds_socket_path = path;
	}

	const char* getSocketPath() {
		return uds_socket_path;
	}

private:
	const char* uds_socket_path;

};

}
