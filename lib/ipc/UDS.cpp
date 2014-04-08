#include <sys/socket.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "SomeIP-common.h"
#include "UDSConnection.h"

namespace SomeIP_Lib {

LOG_DECLARE_DEFAULT_CONTEXT(udsLogContext, "UDS", "UDS");

IPCOperationReport SocketStreamConnection::readBytesBlocking(void* buffer, size_t length) {

	size_t receivedBytes = 0;
	char* b = reinterpret_cast<char*>(buffer);

	while (receivedBytes < length) {

		int n = recv(getFileDescriptor(), b + receivedBytes, length - receivedBytes, 0);

		if (n == 0) {
			// this is not supposed to happen
			assert(false);
			//			onReceiverCongestion();
			//			usleep(100000);
			//			log_error("No byte read. We are probably disconnected.");
			//			return IPCOperationReport::DISCONNECTED;
		}

		if (n < 0)
			if (errno != EAGAIN) {
				log_error("Error reading from socket");
				usleep(1000 * 10);
				disconnect();
				return IPCOperationReport::DISCONNECTED;
			}

		if (n > 0)
			receivedBytes += n;
	}

	return IPCOperationReport::OK;
}

IPCOperationReport UDSConnection::readBlocking(IPCInputMessage& msg) {
	return read(msg, true);
}

IPCOperationReport SocketStreamConnection::readAvailableData(void* buffer, size_t bytesCount, size_t& readBytes) {

	readBytes = 0;

	int n = recv(getFileDescriptor(), buffer, bytesCount, MSG_DONTWAIT);

	if (n < 0) {
		if (errno != EAGAIN) {
			disconnect();
			return IPCOperationReport::DISCONNECTED;
		}
	}

	if (n >= 0) {
		readBytes = n;
	}

	return IPCOperationReport::OK;
}

IPCOperationReport UDSConnection::read(IPCInputMessage& msg, bool blocking) {

	if ( !m_messageLengthReader.isComplete() ) {
		returnIfError( m_messageLengthReader.read(getFileDescriptor(), blocking) );

		if ( m_messageLengthReader.isComplete() )
			msg.setLength( msg.getLength() );  // to make sure the buffer is allocated

	}

	if ( m_messageLengthReader.isComplete() ) {

		size_t bytesToRead = msg.getLength() - msg.getReceivedSize();
		if (blocking) {
			returnIfError( readBytesBlocking(msg.getPayload().getData() + msg.getReceivedSize(), bytesToRead) );
			msg.m_receivedSize = msg.getLength();
		} else {
			size_t readBytes;
			auto p = msg.getPayload().getData();

			returnIfError( readAvailableData(p + msg.getReceivedSize(), bytesToRead, readBytes) );
			msg.m_receivedSize += readBytes;
		}

	}

	return IPCOperationReport::OK;
}

IPCOperationReport UDSConnection::readNonBlocking(IPCInputMessage& msg) {
	return read(msg, false);
}

IPCOperationReport SocketStreamConnection::writeBytesBlocking(const void* buffer, ssize_t length) {
	increaseWrittenBytesCounter(length);

	ssize_t sentBytes = 0;

	while (sentBytes < length) {
		auto n = send(getFileDescriptor(), static_cast<const char*>(buffer) + sentBytes, length - sentBytes, MSG_DONTWAIT);

		if (n < 0)
			if (errno != EAGAIN) {
				disconnect();
				return IPCOperationReport::DISCONNECTED;
			}

		if (n != length - sentBytes) {
			log_verbose("Reception buffer is full");
			onCongestionDetected();
		}

		if (n > 0) {
			sentBytes += n;
		}

	}

	log_verbose( "Bytes written : %s", byteArrayToString(buffer, length).c_str() );

	return IPCOperationReport::OK;
}

void setSocketBufferSize(int fd, int size) {
	int buffsize = size;
	int actualBufferSize;
	socklen_t actualBufferSizeSizeOf = sizeof(actualBufferSize);

	if ( setsockopt( fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize) ) )
		throw Exception("Can't set socket buffer size");

	getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &actualBufferSize, &actualBufferSizeSizeOf);
	if (actualBufferSize < size)
		log_error("Send socket buffer size could not be set.");

	if ( setsockopt( fd, SOL_SOCKET, SO_RCVBUF, &buffsize, sizeof(buffsize) ) )
		throw Exception("Can't set socket buffer size");

	getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &actualBufferSize, &actualBufferSizeSizeOf);
	if (actualBufferSize < size)
		log_error("Send socket buffer size could not be set.");
}

IPCOperationReport UDSConnection::writeBlocking(const IPCMessage& msg) {

	// write length
	auto size = msg.getPayload().size();
	returnIfError( writeBytesBlocking( &size, sizeof(size) ) );

	// write payload
	returnIfError( writeBytesBlocking( msg.getPayload().getData(), msg.getPayload().size() ) );

	log_verbose( "Written IPCMessage : %s", msg.toString().c_str() );

	return IPCOperationReport::OK;
}

IPCOperationReport UDSConnection::writeNonBlocking(const IPCMessage& msg) {

	auto size = msg.getPayload().size();

	if ( isCongested() ) {
		enqueueData( &size, sizeof(size) );
		enqueueData(msg.getPayload().getData(), size);
		return IPCOperationReport::BUFFER_FULL;
	} else {

		switch ( writeBytesNonBlocking( &size, sizeof(size) ) ) {
		case IPCOperationReport::OK : {
			return writeBytesNonBlocking(msg.getPayload().getData(), size);
		}
		break;

		case IPCOperationReport::BUFFER_FULL : {
			// the data which could not fit has been copied in the buffer to be sent later already
			enqueueData(msg.getPayload().getData(), size);
			return IPCOperationReport::BUFFER_FULL;
		}
		break;

		case IPCOperationReport::DISCONNECTED : {
			disconnect();
			return IPCOperationReport::DISCONNECTED;
		}
		break;

		default : {
			assert(false);
		}
		break;

		}
	}

	return IPCOperationReport::DISCONNECTED;
}

IPCOperationReport SocketStreamConnection::writeBytesNonBlocking(const void* data, ssize_t length) {
	increaseWrittenBytesCounter(length);

	const char* dataAsChar = reinterpret_cast<const char*>(data);
	auto writtenBytesCount = send(getFileDescriptor(), dataAsChar, length, MSG_DONTWAIT);
	bool bCongestionDetected = false;
	if (writtenBytesCount == length)
		return IPCOperationReport::OK;
	else if (writtenBytesCount < 0) {
		switch (errno) {

		case EAGAIN :
			log_info( "Congestion detected fileDescriptor: %i", getFileDescriptor() );
			bCongestionDetected = true;
			writtenBytesCount = 0;
			break;

		case EPIPE :
			disconnect();
			return IPCOperationReport::DISCONNECTED;
			break;

		case EBADF :
			log_error( "Bad file descriptor %i", getFileDescriptor() );
			break;

		default :
			log_error( "Unknown error : %i . %s", errno, toString().c_str() );
			disconnect();
			return IPCOperationReport::DISCONNECTED;
			break;
		}
	} else
		bCongestionDetected = true;

	if (bCongestionDetected) {
		enqueueData(dataAsChar + writtenBytesCount, length - writtenBytesCount);
		return IPCOperationReport::BUFFER_FULL;
	}

	return IPCOperationReport::OK;
}

}
