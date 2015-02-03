#pragma once

#include "SomeIP-Config.h"
#include "SomeIP.h"
#include "SomeIP-log.h"
#include <poll.h>
#include <memory>

#include <ByteArray.h>

//#define ENABLE_PING

namespace SomeIP_Lib {

using namespace SomeIP;
using namespace SomeIP_utils;


enum class WatchStatus {
	KEEP_WATCHING, STOP_WATCHING
};


class IdleMainLoopHook {
public:
	typedef std::function<bool ()> CallBackFunction;

	virtual ~IdleMainLoopHook() {
	}

	virtual void activate() = 0;
};

class TimeOutMainLoopHook {
public:
	typedef std::function<void ()> CallBackFunction;

	virtual ~TimeOutMainLoopHook() {
	}

	virtual void activate() = 0;

};

class WatchMainLoopHook {
public:
	typedef std::function<void ()> CallBackFunction;

	virtual ~WatchMainLoopHook() {
	}

	virtual void disable() = 0;

	virtual void enable() = 0;

};

struct MainLoopInterface {

	virtual ~MainLoopInterface() {
	}

	virtual std::unique_ptr<IdleMainLoopHook> addIdle(IdleMainLoopHook::CallBackFunction callBackFunction) = 0;

	virtual std::unique_ptr<TimeOutMainLoopHook> addTimeout(TimeOutMainLoopHook::CallBackFunction,
								int durationInMilliseconds) = 0;

	virtual std::unique_ptr<WatchMainLoopHook> addFileDescriptorWatch(WatchMainLoopHook::CallBackFunction, const pollfd& fd) = 0;

};

typedef MainLoopInterface MainLoopContext;

enum class SomeIPReturnCode {
	OK, DISCONNECTED, ERROR, ALREADY_CONNECTED
};

inline bool isError(SomeIPReturnCode code) {
	return (code != SomeIPReturnCode::OK);
}

typedef SomeIP::ServiceIDs ServiceRegistryEntry;

}

namespace SomeIP_Dispatcher {
using namespace SomeIP;
using namespace SomeIP_utils;
using namespace SomeIP_Lib;
}
