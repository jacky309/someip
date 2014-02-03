#include "ipc.h"
#include "log.h"
#include "Message.h"

LOG_DECLARE_CONTEXT(ipcLogContext, "ipc", "ipc");

IPCRequestID IPCOutputMessage::nextAvailableRequestID = 0x1111;

InputMessage readMessageFromIPCMessage(const IPCInputMessage& inputMsg) {
	InputMessage msg(inputMsg);
	return msg;
}
