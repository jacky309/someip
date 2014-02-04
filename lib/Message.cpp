#include "Message.h"
#include "SomeIP-Serialization.h"
#include "ServiceDiscovery.h"

namespace SomeIP_Lib {
uint16_t OutputMessage::s_nextAvailableRequestID = 0x1000;
std::mutex OutputMessage::s_nextAvailableRequestIDMutex;

bool SomeIPServiceDiscoveryMessage::s_MulticastReboot = true;
bool SomeIPServiceDiscoveryMessage::s_UnicastReboot = true;
uint16_t SomeIPServiceDiscoveryMessage::s_nextMulticastSessionID = 0x0001;
uint16_t SomeIPServiceDiscoveryMessage::s_nextUnicastSessionID = 0x0001;
}
