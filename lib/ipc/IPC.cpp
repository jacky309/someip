#include "SomeIP-common.h"
#include "ipc.h"
#include "Message.h"

namespace SomeIP_Lib {

LOG_DECLARE_CONTEXT(ipcLogContext, "ipc", "ipc");

IPCRequestID IPCOutputMessage::nextAvailableRequestID = 0x1111;

InputMessage readMessageFromIPCMessage(const IPCInputMessage& inputMsg) {
	InputMessage msg(inputMsg);
	return msg;
}

}
