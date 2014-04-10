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
		//		uds_socket_path = DEFAULT_SERVER_SOCKET_PATH;
	}

	virtual ~UDSConnection() {
	}

	SomeIPReturnCode connectToServer(const char* uds_socket_path) {

		int fileDescriptor = socket(AF_UNIX, SOCK_STREAM, 0);

		if ( fileDescriptor == -1 ) {
			log_warning("Failed to create socket");
			return SomeIPReturnCode::ERROR;
		}

		struct sockaddr_un remote;

		remote.sun_family = AF_UNIX;
		strcpy(remote.sun_path, uds_socket_path);

		if (::connect( fileDescriptor, (struct sockaddr*) &remote, strlen(remote.sun_path) +
			       sizeof(remote.sun_family) )
		    == -1) {
			log_warning() << "Failed to connect to the daemon via socket " << uds_socket_path <<
			". Trying alternative socket";
			if (alternative_uds_socket_path != nullptr) {
				uds_socket_path = alternative_uds_socket_path;
				strcpy(remote.sun_path, uds_socket_path);
				if (::connect( fileDescriptor, (struct sockaddr*) &remote,
					       strlen(remote.sun_path) + sizeof(remote.sun_family) ) == -1) {
					log_error() << "Failed to connect to daemon via socket " << uds_socket_path;
					return SomeIPReturnCode::ERROR;
				}
			}
		}

		setFileDescriptor(fileDescriptor);
		return SomeIPReturnCode::OK;
	}

	virtual void handleIncomingIPCMessage(IPCInputMessage& inputMessage) = 0;

	void setInputMessage(IPCInputMessage& msg) {
		m_currentInputMessage = &msg;
		msg.clear();
		m_messageLengthReader.setBuffer( &msg.m_totalMessageSize, sizeof(msg.m_totalMessageSize) );
	}

private:
	IPCOperationReport read(IPCInputMessage& msg, bool blocking);

	//	const char* uds_socket_path = nullptr;
	const char* alternative_uds_socket_path = nullptr;
	IPCBufferReader m_messageLengthReader;

protected:
	IPCInputMessage* m_currentInputMessage = nullptr;

};


class UDSServer : public SocketStreamServer {

	LOG_DECLARE_CLASS_CONTEXT("UDSS", "UDSServer");

public:
	UDSServer() {
	}

	IPCReturnCode initServerSocket(const char* uds_socket_path, int fd = 0) {

		m_fileDescriptor = fd;

		if (getFileDescriptor() == 0) {

			if ( ( m_fileDescriptor = socket(AF_UNIX, SOCK_STREAM, 0) ) < 0 ) {
				log_error() << "Failed to open socket " << uds_socket_path;
				return IPCReturnCode::ERROR;
			}

			struct sockaddr_un local = {.sun_family = AF_UNIX};

			strcpy(local.sun_path, uds_socket_path);
			unlink(local.sun_path);

			if (::bind( getFileDescriptor(), (struct sockaddr*) &local,
				    strlen(local.sun_path) + sizeof(local.sun_family) ) != 0) {
				log_warning() << "Failed to bind server socket " << uds_socket_path;

				//				uds_socket_path = alternative_uds_socket_path;
				//				strcpy(local.sun_path, uds_socket_path);
				//				unlink(local.sun_path);
				//				if (::bind( getFileDescriptor(), (struct sockaddr*) &local,
				//					    strlen(local.sun_path) + sizeof(local.sun_family) ) != 0) {
				//					log_warning() << "Failed to bind server socket " << uds_socket_path;
				//					throw ConnectionException("Failed to open socket");
				//				}

				return IPCReturnCode::ERROR;
			}

			if (listen(getFileDescriptor(), SOMAXCONN) != 0) {
				log_error() << "Failed to listen to socket " << uds_socket_path;
				throw ConnectionException("Failed to listen");
			}

			m_uds_socket_path = uds_socket_path;
			return IPCReturnCode::OK;

		}
	}

	//	void setSocketPath(const char* path) {		uds_socket_path = path;	}

	const char* getSocketPath() {
		return m_uds_socket_path;
	}

private:
	const char* m_uds_socket_path = nullptr;

};

}
