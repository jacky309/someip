#include "SomeIP-clientLib.h"

namespace SomeIPClient {

LOG_DECLARE_CONTEXT(clientLibContext, "SOCL", "SomeIP Client lib");


std::string ClientConnection::getDaemonStateDump() {
	IPCOutputMessage msg(IPCMessageType::DUMP_STATE);
	IPCInputMessage returnMessage = writeRequest(msg);

	if (returnMessage.getReturnCode() == IPCReturnCode::ERROR)
		throw Exception("Can't dump state");

	IPCInputMessageReader reader(returnMessage);

	std::string s;
	reader >> s;

	return s;
}

bool ClientConnection::dispatchIncomingMessages() {
	dispatchQueuedMessages();
	return readIncomingMessages([&] (IPCInputMessage & msg) {
					    onIPCInputMessage(msg);
					    return true;
				    });
}

void ClientConnection::connect(ClientConnectionListener& clientReceiveCb) {

	if ( isConnected() )
		throw ConnectionException("Already connected !");

	messageReceivedCallback = &clientReceiveCb;

	swapInputMessage();

	connectToServer();

	log_info() << "We are connected to the dispatcher";

}

void ClientConnection::registerService(SomeIP::ServiceID serviceID) {
	IPCOutputMessage msg(IPCMessageType::REGISTER_SERVICE);
	msg << serviceID;
	IPCInputMessage returnMessage = writeRequest(msg);

	if (returnMessage.getReturnCode() == IPCReturnCode::ERROR)
		throw RegistrationException("Can't register service", serviceID);
	else
		log_info("Successfully registered service 0x%X", serviceID);
}

void ClientConnection::unregisterService(SomeIP::ServiceID serviceID) {
	IPCOutputMessage msg(IPCMessageType::UNREGISTER_SERVICE);
	msg << serviceID;
	IPCInputMessage returnMessage = writeRequest(msg);

	if (returnMessage.getReturnCode() == IPCReturnCode::ERROR)
		throw RegistrationException("Can't unregister service", serviceID);
	else
		log_info("Successfully unregistered service 0x%X", serviceID);
}

void ClientConnection::subscribeToNotifications(SomeIP::MessageID messageID) {
	log_debug("Subscribing to notifications 0x%X", messageID);
	IPCOutputMessage msg(IPCMessageType::SUBSCRIBE_NOTIFICATION);
	msg << messageID;
	writeMessage(msg);
}

SomeIPFunctionReturnCode ClientConnection::sendMessage(OutputMessage& msg) {
	const IPCMessage& ipcMessage = msg.getIPCMessage();
	writeMessage(ipcMessage);
#ifdef ENABLE_TRAFFIC_LOGGING
	log_debug( "Message sent : %s", msg.toString().c_str() );
#endif
	return SomeIPFunctionReturnCode::OK;
}


InputMessage ClientConnection::sendMessageBlocking(const OutputMessage& msg) {
	assert( msg.getHeader().isRequestWithReturn() );
	assert(msg.getClientIdentifier() == 0);

#ifdef ENABLE_TRAFFIC_LOGGING
	log_verbose( "Send blocking message : %s", msg.toString().c_str() );
#endif

	const IPCMessage& ipcMessage = msg.getIPCMessage();

	std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex); // we prevent other threads to steal the response of our request
	writeMessage(ipcMessage);

	return waitForAnswer(msg);
}

InputMessage ClientConnection::waitForAnswer(const OutputMessage& requestMsg) {
	std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex); // we prevent other threads to steal the response of our request

	InputMessage answerMessage;

	log_debug("Waiting for answer to message : ") << requestMsg.toString();

	readIncomingMessagesBlocking([&] (IPCInputMessage & incomingIPCMsg) {

					     if (incomingIPCMsg.getMessageType() == IPCMessageType::SEND_MESSAGE) {
						     InputMessage inputMsg = readMessageFromIPCMessage(incomingIPCMsg);
						     if ( inputMsg.isAnswerTo(requestMsg) ) {

	                                                     // copy the content into the message to return to the caller
							     answerMessage.copyFrom(incomingIPCMsg);
#ifdef ENABLE_TRAFFIC_LOGGING
							     log_debug( "Answer received : %s", inputMsg.toString().c_str() );
#endif
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

void ClientConnection::pushToQueue(const IPCInputMessage& msg) {
	m_queue.push(msg);

	log_verbose("Message pushed : ") << msg.toString().c_str();

	if (m_mainLoop)
		m_mainLoop->addIdleCallback([&] () {
						    dispatchQueuedMessages();
					    });

}

IPCInputMessage ClientConnection::writeRequest(IPCOutputMessage& ipcMessage) {
	IPCInputMessage answerMessage;
	std::lock_guard<std::recursive_mutex> emissionLock(dataEmissionMutex);
	std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex); // we prevent other threads to steal the response of our request

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

void ClientConnection::dispatchQueuedMessages() {
	const IPCInputMessage* msg = NULL;
	//		log_debug("dispatchQueuedMessages");
	do {
		{
			std::lock_guard<std::recursive_mutex> receptionLock(dataReceptionMutex);
			msg = m_queue.pop();
		}

		if (msg != NULL) {
			handleConstIncomingIPCMessage(*msg);
			delete msg;
		}
	} while (msg != NULL);
}

void ClientConnection::handleConstIncomingIPCMessage(const IPCInputMessage& inputMessage) {

	IPCInputMessageReader reader(inputMessage);

	auto messageType = inputMessage.getMessageType();

	switch (messageType) {

	case IPCMessageType::SEND_MESSAGE : {
		const InputMessage msg = readMessageFromIPCMessage(inputMessage);

#ifdef ENABLE_TRAFFIC_LOGGING
		log_debug( "Dispatching message %s:", msg.toString().c_str() );
#endif
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
		log_verbose("PING RECEIVED");
	}
	break;

	case IPCMessageType::SERVICES_REGISTERED : {
		while ( reader.remainingBytesCount() >= sizeof(SomeIP::ServiceID) ) {
			SomeIP::ServiceID serviceID;
			reader >> serviceID;
			getServiceRegistry().onServiceRegistered(serviceID);
		}
	}
	break;

	case IPCMessageType::SERVICES_UNREGISTERED : {
		while ( reader.remainingBytesCount() >= sizeof(SomeIP::ServiceID) ) {
			SomeIP::ServiceID serviceID;
			reader >> serviceID;
			getServiceRegistry().onServiceUnregistered(serviceID);
		}
	}
	break;

	default : {
		log_error( "Unknown message type : %i", static_cast<int>(messageType) );
	}
	break;
	}
}


IPCOperationReport ClientConnection::readIncomingMessagesBlocking(IPCMessageReceivedCallbackFunction dispatchFunction) {

	bool bKeepProcessing;

	do {

		IPCInputMessage* msg = NULL;

		{
			std::unique_lock<std::recursive_mutex> receptionLock(dataReceptionMutex);
			returnIfError( readBlocking(*m_currentInputMessage) );
			msg = m_currentInputMessage;
			swapInputMessage();
		}

		bKeepProcessing = dispatchFunction(*msg);

		delete msg;

	} while (bKeepProcessing);

	return IPCOperationReport::OK;
}

bool ClientConnection::readIncomingMessages(IPCMessageReceivedCallbackFunction dispatchFunction) {

	bool bKeepProcessing = true;

	do {

		IPCInputMessage* msg = NULL;

		{
			std::unique_lock<std::recursive_mutex> receptionLock(dataReceptionMutex);
			readNonBlocking(*m_currentInputMessage);
			if ( m_currentInputMessage->isComplete() ) {
				msg = m_currentInputMessage;
				swapInputMessage();
			}
		}

		if (msg != NULL) {
			bKeepProcessing = dispatchFunction(*msg);
			delete msg;

		} else
			bKeepProcessing = false;

	} while (bKeepProcessing);

	return true;
}


void ClientConnection::onCongestionDetected() {
	enqueueIncomingMessages();

	// we block until we can read or write to the socket
	struct pollfd fd;
	fd.fd = getFileDescriptor();
	fd.events = POLLIN | POLLOUT | POLLHUP;
	poll(&fd, 1, 1000);

}

void ClientConnection::onDisconnected() {
	log_warning("Disconnected from server");
	messageReceivedCallback->onDisconnected();
}


}
