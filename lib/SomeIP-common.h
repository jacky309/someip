#pragma once

#include "SomeIP-Config.h"
#include "SomeIP.h"
#include "utilLib/SomeIP-log.h"

namespace SomeIP_Lib {
using namespace SomeIP;
using namespace SomeIP_utils;

enum class SomeIPReturnCode {
	OK, DISCONNECTED, ERROR, ALREADY_CONNECTED
};

inline bool isError(SomeIPReturnCode code) {
	return (code != SomeIPReturnCode::OK);
}

}

namespace SomeIP_Dispatcher {
using namespace SomeIP;
using namespace SomeIP_utils;
using namespace SomeIP_Lib;
}
