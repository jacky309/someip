#include "SomeIP-clientLib.h"
#include <unistd.h>
#include <fcntl.h>

namespace SomeIPClient {

LOG_DECLARE_CONTEXT(clientLibContext, "SOCL", "SomeIP Client lib");


SomeIPReturnCode ClientDaemonConnection::getDaemonStateDump(std::string& dump) {
	IPCOutputMessage msg(IPCMessageType::DUMP_STATE);
	IPCInputMessage returnMessage = writeRequest(msg);

	if ( returnMessage.isError() )
		return SomeIPReturnCode::DISCONNECTED;

	IPCInputMessageReader reader(returnMessage);

	reader >> dump;

	return SomeIPReturnCode::OK;
}

bool ClientDaemonConnection::dispatchIncomingMessages() {
	dispatchQueuedMessages();
	return readIncomingMessages([&] (IPCInputMessage & msg) {
					    onIPCInputMessage(msg);
					    return true;
				    });
}

SomeIPReturnCode ClientDaemonConnection::connect(ClientConnectionListener& clientReceiveCb) {

	if (m_mainLoop == nullptr)
		log_error() <<
		"You need to provide a main loop integration object (setMainLoopInterface()) before calling connect()";

	if ( isConnected() )
		return SomeIPReturnCode::ALREADY_CONNECTED;

	messageReceivedCallback = &clientReceiveCb;

	newInputMessage();

	auto c = connectToServer(DEFAULT_SERVER_SOCKET_PATH);

	if ( !isError(c) ) {
		log_info() << "Connected to the dispatcher";

		struct pollfd fd;
		fd.fd = getFileDescriptor();
		fd.revents = 0;
		fd.events = POLLIN;
		m_inputDataWatch = m_mainLoop->addFileDescriptorWatch([&] () {
								dispatchIncomingMessages();
							}, fd);

		m_inputDataWatch->enable();

		int returnCode = pipe(m_queuedMessageIndicatorPipe);
		assert(returnCode == 0);
		fd.fd = m_queuedMessageIndicatorPipe[0];

		m_bufferedMessagesWatch = m_mainLoop->addFileDescriptorWatch([&] () {
			 dispatchQueuedMessages();
							    }, fd);

		// set the pipe to non-blocking
		int flags = fcntl(m_queuedMessageIndicatorPipe[0], F_GETFL, 0);
		fcntl(m_queuedMessageIndicatorPipe[0], F_SETFL, flags | O_NONBLOCK);

		m_bufferedMessagesWatch->enable();

		fd.events = POLLHUP;
		m_disconnectionWatch = m_mainLoop->addFileDescriptorWatch([&] () {
								    onDisconnected();
							    }, fd);
		m_disconnectionWatch->enable();

	}

	return c;
}

SomeIPReturnCode ClientDaemonConnection::registerService(SomeIP::ServiceIDs serviceID) {
	IPCOutputMessage msg(IPCMessageType::REGISTER_SERVICE);
	msg << serviceID.serviceID << serviceID.instanceID;
	IPCInputMessage returnMessage = writeRequest(msg);

	if ( returnMessage.isError() )
		return SomeIPReturnCode::ERROR;
	else {
		log_info() << "Successfully registered service " << serviceID.toString();
		return SomeIPReturnCode::OK;
	}
}

SomeIPReturnCode ClientDaemonConnection::unregisterService(SomeIP::ServiceIDs serviceID) {
	IPCOutputMessage msg(IPCMessageType::UNREGISTER_SERVICE);
	msg << serviceID.serviceID << serviceID.instanceID;
	IPCInputMessage returnMessage = writeRequest(msg);

	if ( returnMessage.isError() )
		return SomeIPReturnCode::ERROR;
	else {
		log_info() << "Successfully unregistered service " << serviceID.toString();
		return SomeIPReturnCode::OK;
	}
}

SomeIPReturnCode ClientDaemonConnection::subscribeToNotifications(SomeIP::MemberIDs member) {
	log_debug() << "Subscribing to notifications " << member.toString();
	IPCOutputMessage msg(IPCMessageType::SUBSCRIBE_NOTIFICATION);
	msg << member.m_serviceIDs.serviceID  << member.m_serviceIDs.instanceID << member.m_memberID;
	return writeMessage(msg);
}

SomeIPReturnCode ClientDaemonConnection::sendMessage(const OutputMessage& msg) {
	const IPCMessage& ipcMessage = msg.getIPCMessage();
	auto ret = writeMessage(ipcMessage);
	log_traffic() << "Message sent : " << msg;
	return ret;
}


SomeIPReturnCode ClientDaemonConnection::sendPing() {
	IPCOutputMessage ipcMessage(IPCMessageType::PONG);
	return writeMessage(ipcMessage);
}

InputMessage ClientDaemonConnection::sendMessageBlocking(const OutputMessage& msg) {
	assert( msg.getHeader().isRequestWithReturn() );
	assert(msg.getClientIdentifier() == 0);

	log_traffic() << "Send blocking message : " << msg.toString();

	const IPCMessage& ipcMessage = msg.getIPCMessage();

	std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex); // we prevent other threads from stealing the response of our request
	writeMessage(ipcMessage);

	return waitForAnswer(msg);
}

bool ClientDaemonConnection::isServiceAvailableBlocking(ServiceIDs service) {
	IPCOutputMessage msg(IPCMessageType::GET_SERVICE_LIST);
	auto inputMessage = writeRequest(msg);
	IPCInputMessageReader reader(inputMessage);

	while(reader.hasMoreData()) {
		ServiceRegistryEntry entry;
		reader >> entry.serviceID >> entry.instanceID;
		if (entry == service)
			return true;
	}

	return false;
}

InputMessage ClientDaemonConnection::waitForAnswer(const OutputMessage& requestMsg) {
	std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex); // we prevent other threads from stealing the response of our request

	InputMessage answerMessage;

	log_debug() << "Waiting for answer to message : " << requestMsg.toString();

	readIncomingMessagesBlocking([&] (IPCInputMessage & incomingIPCMsg) {

					     if (incomingIPCMsg.getMessageType() == IPCMessageType::SEND_MESSAGE) {
						     InputMessage inputMsg = readMessageFromIPCMessage(incomingIPCMsg);
						     if ( inputMsg.isAnswerTo(requestMsg) ) {

	                                                     // copy the content into the message to return to the caller
							     answerMessage.copyFrom(incomingIPCMsg);
							     log_traffic() << "Answer received : " << inputMsg.toString();
							     return false;
						     } else {
							     pushToQueue(incomingIPCMsg);
							     return true;
						     }
					     }

					     return true;
				     });

	return answerMessage;
}

void ClientDaemonConnection::pushToQueue(const IPCInputMessage& msg) {
	m_queue.push(msg);
	log_verbose() << "Message pushed : " << msg.toString();

	// notify ourself that we have a message to handle first
	write(m_queuedMessageIndicatorPipe[1], &m_dummy, sizeof(m_dummy));
}

IPCInputMessage ClientDaemonConnection::writeRequest(IPCOutputMessage& ipcMessage) {
	IPCInputMessage answerMessage;
	std::lock_guard<std::recursive_mutex> emissionLock(dataEmissionMutex);
	std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex); // we prevent other threads from stealing the response of our request

	ipcMessage.assignRequestID();

	writeBlocking(ipcMessage);

	auto returnCode = readIncomingMessagesBlocking([&] (IPCInputMessage & incomingMsg) {

							       if ( incomingMsg.isResponseOf(ipcMessage) ) {
								       answerMessage = incomingMsg;
								       return false;
							       } else {
								       pushToQueue(incomingMsg);
								       return true;
							       }

						       });

	if (returnCode != IPCOperationReport::OK)
		answerMessage.setError();

	return answerMessage;
}

void ClientDaemonConnection::dispatchQueuedMessages() {

	::read(m_queuedMessageIndicatorPipe[0], &m_dummy, sizeof(m_dummy));

	const IPCInputMessage* msg = nullptr;

	do {
		{
			std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex);
			msg = m_queue.pop();
		}

		if (msg != nullptr) {
			handleConstIncomingIPCMessage(*msg);
			delete msg;
		}
	} while (msg != nullptr);
}

void ClientDaemonConnection::handleConstIncomingIPCMessage(const IPCInputMessage& inputMessage) {

	IPCInputMessageReader reader(inputMessage);

	auto messageType = inputMessage.getMessageType();

	switch (messageType) {

	case IPCMessageType::SEND_MESSAGE : {
		const InputMessage msg = readMessageFromIPCMessage(inputMessage);
		log_traffic() << "Dispatching message " << msg.toString();
		if (m_endPoint.processMessage(msg) == MessageProcessingResult::NotProcessed_OK)
			messageReceivedCallback->processMessage(msg);
	}
	break;

	case IPCMessageType::PING : {
		for (size_t i = 0; i < inputMessage.getUserDataLength(); i++) {
			uint8_t v;
			reader >> v;
			//				assert(v == 0x11);
			assert( v == static_cast<uint8_t>(i) );
		}
		IPCOutputMessage ipcMessage(IPCMessageType::PONG);
		writeMessage(ipcMessage);
		log_verbose() << "PING RECEIVED";
	}
	break;

	case IPCMessageType::SERVICES_REGISTERED : {
		while ( reader.remainingBytesCount() >= sizeof(SomeIP::ServiceID) ) {
			SomeIP::ServiceIDs serviceID;
			reader >> serviceID.serviceID >> serviceID.instanceID;
			onServiceRegistered(serviceID);
		}
	}
	break;

	case IPCMessageType::SERVICES_UNREGISTERED : {
		while ( reader.remainingBytesCount() >= sizeof(SomeIP::ServiceID) ) {
			SomeIP::ServiceIDs serviceID;
			reader >> serviceID.serviceID >> serviceID.instanceID ;
			onServiceUnregistered(serviceID);
		}
	}
	break;

	default : {
		log_error() << "Unknown message type : " << static_cast<int>(messageType);
	}
	break;
	}
}


IPCOperationReport ClientDaemonConnection::readIncomingMessagesBlocking(IPCMessageReceivedCallbackFunction dispatchFunction) {

	bool bKeepProcessing;

	do {

		IPCInputMessage* msg = nullptr;

		{
			std::unique_lock<std::recursive_mutex> receptionLock(dataReceptionMutex);
			returnIfError( readBlocking(*m_currentInputMessage) );
			msg = m_currentInputMessage;
			newInputMessage();
		}

		bKeepProcessing = dispatchFunction(*msg);

		delete msg;

	} while (bKeepProcessing);

	return IPCOperationReport::OK;
}

bool ClientDaemonConnection::readIncomingMessages(IPCMessageReceivedCallbackFunction dispatchFunction) {

	bool bKeepProcessing = true;

	do {

		IPCInputMessage* msg = nullptr;

		{
			std::unique_lock<std::recursive_mutex> receptionLock(dataReceptionMutex);
			readNonBlocking(*m_currentInputMessage);
			if ( m_currentInputMessage->isComplete() ) {
				msg = m_currentInputMessage;
				newInputMessage();
			}
		}

		if (msg != nullptr) {
			bKeepProcessing = dispatchFunction(*msg);
			delete msg;
		} else
			bKeepProcessing = false;

	} while (bKeepProcessing);

	return true;
}


void ClientDaemonConnection::onCongestionDetected() {
	enqueueIncomingMessages();

	// we block until we can read or write to the socket
	struct pollfd fd;
	fd.fd = getFileDescriptor();
	fd.events = POLLIN | POLLOUT | POLLHUP;
	poll(&fd, 1, 1000);
}

void ClientDaemonConnection::onDisconnected() {
	log_warning() << "Disconnected from server";
	if (messageReceivedCallback)
		messageReceivedCallback->onDisconnected();
}


}
