#pragma once

#include <functional>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <dirent.h>
#include <unistd.h>

#include "ipc.h"

#define returnIfError(code) {IPCOperationReport c = code; if (c != IPCOperationReport::OK) return c; }

class IPCBufferReader {

public:
	IPCBufferReader() {
	}

	IPCBufferReader(void* pBuffer, size_t size) {
		setBuffer(pBuffer, size);
	}

	void setBuffer(void* pBuffer, size_t size) {
		m_pBuffer = (char*) pBuffer;
		m_size = size;
		m_receivedBytesCount = 0;
	}

	IPCOperationReport read(int fd, bool blocking = false) {

		if (m_size - m_receivedBytesCount == 0)
			return IPCOperationReport::OK;  // Nothing to read

		ssize_t readLength = recv(fd, m_pBuffer + m_receivedBytesCount, m_size - m_receivedBytesCount,
					  blocking ? 0 : MSG_DONTWAIT);

		if (readLength < 0)
			return IPCOperationReport::DISCONNECTED;
		else if (readLength > 0) {
			//			log_verbose( "Read %i bytes from socket : %s", readLength, bytesToString(m_pBuffer + m_receivedBytesCount, readLength));
			m_receivedBytesCount += readLength;
		} else {
			if (blocking)                            // no byte received as we are in blocking mode => error
				return IPCOperationReport::DISCONNECTED;
		}

		return IPCOperationReport::OK;
	}

	bool isComplete() {
		return (m_receivedBytesCount == m_size);
	}

	void clear() {
		m_receivedBytesCount = 0;
	}

private:
	char* m_pBuffer = nullptr;
	size_t m_receivedBytesCount = 0;
	size_t m_size = -1;

};

/**
 * Handles a stream connection.
 * Both blocking and non-blocking calls are offered. When non-blocking calls are used, a local buffer is used to contain the
 * data which could not be written to the socket (congestion). When trying to send some additionnal data, the data which could
 * not be sent is sent first, so that the data order is always kept.
 */
class SocketStreamConnection {

	LOG_DECLARE_CLASS_CONTEXT("SSC", "SocketStreamConnection");

public:
	static const int UNINITIALIZED_FILE_DESCRIPTOR = -1;

	SocketStreamConnection() {
	}

	virtual ~SocketStreamConnection() {
	}

	bool isConnected() const {
		return (connectionFileDescriptor != UNINITIALIZED_FILE_DESCRIPTOR);
	}

	void disconnect() {
		if ( isConnected() ) {
			close( getFileDescriptor() );
			setFileDescriptor(UNINITIALIZED_FILE_DESCRIPTOR);
			onDisconnected();
		}
	}

	int getFileDescriptor() const {
		return connectionFileDescriptor;
	}

	void setFileDescriptor(int fd) {
		connectionFileDescriptor = fd;
	}

	/**
	 * Returns true if some bytes are available to read
	 */
	bool hasAvailableBytes() {
		int bytes_available;
		ioctl(getFileDescriptor(), FIONREAD, &bytes_available);
		return (bytes_available != 0);
	}

	/**
	 * Called whenever the connection has been broken
	 */
	virtual void onDisconnected() = 0;

	virtual void onCongestionDetected() = 0;

	virtual void onCongestionFinished() {
		log_info( "Congestion finished %s", toString().c_str() );
	}

	/**
	 */
	IPCOperationReport writePendingDataNonBlocking() {
		if (m_dataToBeSent.size() != 0) {
			ByteArray localContentCopy = m_dataToBeSent;
			m_dataToBeSent.resize(0);
			auto v = writeBytesNonBlocking( localContentCopy.getData(), localContentCopy.size() );
			if (v == IPCOperationReport::OK)
				onCongestionFinished();
			return v;
		}
		return IPCOperationReport::OK;
	}

protected:
	IPCOperationReport readBytesBlocking(void* buffer, size_t length);
	IPCOperationReport writeBytesBlocking(const void* buffer, ssize_t length);
	IPCOperationReport writeBytesNonBlocking(const void* data, ssize_t length);
	IPCOperationReport readAvailableData(void* buffer, size_t bytesCount, size_t& readBytes);

	virtual std::string toString() const = 0;

	void enqueueData(const void* data, size_t length) {
		if (length == 4)
			log_verbose( "Appended data to outgoing buffer : %s", byteArrayToString(data, length).c_str() );

		m_dataToBeSent.append(data, length);
		onCongestionDetected();
	}

	bool isCongested() {
		return (m_dataToBeSent.size() != 0);
	}

private:
	void increaseWrittenBytesCounter(size_t count) {
		m_writtenBytesCount += count;
		if ( ( (m_writtenBytesCount / 1000) % 1000 ) == 0 )
			log_verbose( "Written %zu bytes to %s", m_writtenBytesCount, toString().c_str() );
	}

	void increaseReadBytesCounter(size_t count) {
		m_receivedBytesCount += count;
		if ( ( (m_receivedBytesCount / 1000) % 1000 ) == 0 )
			log_verbose( "Received %zu bytes from %s", m_receivedBytesCount, toString().c_str() );
	}

	//	const char* uds_socket_path;
	int connectionFileDescriptor = UNINITIALIZED_FILE_DESCRIPTOR;
	ByteArray m_dataToBeSent;

	size_t m_writtenBytesCount = 0;
	size_t m_receivedBytesCount = 0;

};

class SocketStreamServer {

	LOG_DECLARE_CLASS_CONTEXT("SocketStreamServer", "SocketStreamServer");

public:
	SocketStreamServer() {
	}

	virtual ~SocketStreamServer() {
	}

	virtual void createNewClientConnection(int fileDescriptor) = 0;

	void refuseConnections() {
		acceptConnections = false;
	}

	bool acceptConnections = true;

	/**
	 * Called when a new client connects to the server socket
	 */
	bool handleNewConnection() {

		if (!acceptConnections)
			return false;

		struct sockaddr remote = { 0 };
		socklen_t sizeOfStruct = sizeof(remote);

		log_info("Accepting new connection");
		int clientConnectionFileDescriptor = accept(m_fileDescriptor, &remote, &sizeOfStruct);
		if (clientConnectionFileDescriptor == -1) {
			log_error("Failed to accept client connection");
			return false;
		}

		createNewClientConnection(clientConnectionFileDescriptor);
		return true;
	}

	int getFileDescriptor() const {
		return m_fileDescriptor;
	}

protected:
	void setFileDescriptor(int fileDescriptor) {
		m_fileDescriptor = fileDescriptor;
	}

	int m_fileDescriptor = SocketStreamConnection::UNINITIALIZED_FILE_DESCRIPTOR;

};
