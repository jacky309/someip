#include "LocalClient.h"

namespace SomeIP_Dispatcher {

void LocalClient::handleIncomingIPCMessage(IPCInputMessage& inputMessage) {
	IPCInputMessageReader reader(inputMessage);

	switch ( inputMessage.getMessageType() ) {

	case IPCMessageType::SEND_MESSAGE : {
		DispatcherMessage msg = readMessageFromIPCMessage(inputMessage);
		processIncomingMessage(msg);
	}
	break;

	case IPCMessageType::REGISTER_SERVICE : {

		SomeIP::ServiceIDs serviceID;
		reader >> serviceID.serviceID;
		reader >> serviceID.instanceID;

		log_debug() << "REGISTER_SERVICE Message received from client " << toString() << ". ServiceID:" << serviceID.toString();
		Service* service = registerService(serviceID, true);

		//		sendPingMessage();

		IPCReturnCode returnCode = (service != nullptr) ? IPCReturnCode::OK : IPCReturnCode::ERROR;

		IPCOutputMessage answer(inputMessage, returnCode);
		writeNonBlocking(answer);
	}
	break;

	case IPCMessageType::UNREGISTER_SERVICE : {

		SomeIP::ServiceIDs serviceID;
		reader >> serviceID.serviceID;
		reader >> serviceID.instanceID;

		log_debug() << "UNREGISTER_SERVICE Message received from client " << toString() << ". ServiceID:" << serviceID.toString();

		IPCReturnCode returnCode = IPCReturnCode::ERROR;

		for (auto service : m_registeredServices) {
			if (service->getServiceIDs() == serviceID) {
				getDispatcher().unregisterService(*service);
				removeFromVector(m_registeredServices, service);
				returnCode = IPCReturnCode::OK;
				break;
			}
		}

		IPCOutputMessage answer(inputMessage, returnCode);
		writeNonBlocking(answer);
	}
	break;

	case IPCMessageType::PONG : {
		log_debug() << "PONG Message received from client " << toString();
		//			sendTestMessage();
	}
	break;

	case IPCMessageType::GET_SERVICE_LIST : {
		log_debug() << "GET_SERVICE_LIST Message received from client " << toString();
		IPCOutputMessage answer(inputMessage, IPCReturnCode::OK);
		for (auto& service : getDispatcher().getServices()) {
			answer << service->getServiceIDs().serviceID << service->getServiceIDs().instanceID;
		}
		writeNonBlocking(answer);
	}
	break;

	case IPCMessageType::SUBSCRIBE_NOTIFICATION : {
		SomeIP::MemberIDs memberIDs;
		reader >> memberIDs.m_serviceIDs.serviceID >> memberIDs.m_serviceIDs.instanceID >> memberIDs.m_memberID;
		subscribeToNotification(memberIDs);
	}
	break;

	case IPCMessageType::DUMP_STATE : {

		log_debug() << "DUMP_STATE Message received from client " << toString();

		IPCOutputMessage answer(inputMessage, IPCReturnCode::OK);
		auto str = getDispatcher().dumpState();
		answer << str;
		writeNonBlocking(answer);

	}
	break;

	default : {
		log_error() << "Unknown message type : " << SomeIP_Lib::toString( inputMessage.getMessageType() );
	}
	break;
	}

}

WatchStatus LocalClient::onIncomingDataAvailable() {

	if ( isInputBlocked() ) {
		return WatchStatus::STOP_WATCHING;
	}

	bool bKeepProcessing = true;

	do {
		readNonBlocking(*m_currentInputMessage);

		IPCInputMessage& inputMessage = *m_currentInputMessage;

		if ( inputMessage.isComplete() ) {
			handleIncomingIPCMessage(inputMessage);
			setInputMessage(inputMessage);
		} else
			bKeepProcessing = false;

	} while (bKeepProcessing);

	return WatchStatus::KEEP_WATCHING;

}

void LocalClient::sendPingMessage() {
#ifdef ENABLE_PING
	log_verbose() << "Sending PING message to " << toString();
	IPCOutputMessage msg(IPCMessageType::PING);
	for (size_t i = 0; i < 256; i++) {
		uint8_t v = i;
		msg << v;
	}

	writeNonBlocking(msg);
#endif
}

void LocalClient::initConnection() {
	pid = getPidFromFiledescriptor( getFileDescriptor() );
	processName = getProcessName(pid);

	{
		pollfd fd;
		fd.fd = getFileDescriptor();
		fd.events = POLLIN;
		m_inputDataWatcher = m_mainLoopContext.addFileDescriptorWatch([&] () {
									return onIncomingDataAvailable();
								}, fd);
		m_inputDataWatcher->enable();
	}

	{
		pollfd fd;
		fd.fd = getFileDescriptor();
		fd.events = POLLOUT;
		m_outputDataWatcher = m_mainLoopContext.addFileDescriptorWatch([&] () {
									 return onWritingPossible();
								 }, fd);
	}

	{
		pollfd fd;
		fd.fd = getFileDescriptor();
		fd.events = POLLHUP;
		m_disconnectionWatcher = m_mainLoopContext.addFileDescriptorWatch([&] () {
									    disconnect();
								    }, fd);
		m_disconnectionWatcher->enable();
	}

	sendPingMessage();
	sendRegistry();
}

}
