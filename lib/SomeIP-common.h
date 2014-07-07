#pragma once

#include "SomeIP-Config.h"
#include "SomeIP.h"
#include "SomeIP-log.h"
#include <poll.h>
#include <memory>

#include <ByteArray.h>

namespace SomeIP_Lib {

using namespace SomeIP;
using namespace SomeIP_utils;


class IdleMainLoopHook {
public:
	typedef std::function<bool (void)> CallBackFunction;

	virtual ~IdleMainLoopHook() {
	}

	virtual void activate() = 0;
};

class TimeOutMainLoopHook {
public:
	typedef std::function<void ()> CallBackFunction;

	virtual ~TimeOutMainLoopHook() {
	}

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

	virtual std::unique_ptr<IdleMainLoopHook> addIdleCallback(IdleMainLoopHook::CallBackFunction callBackFunction) = 0;

	virtual std::unique_ptr<TimeOutMainLoopHook> addTimeout(TimeOutMainLoopHook::CallBackFunction,
								int durationInMilliseconds) = 0;

	virtual std::unique_ptr<WatchMainLoopHook> addWatch(WatchMainLoopHook::CallBackFunction, const pollfd& fd) = 0;

};

typedef MainLoopInterface MainLoopContext;

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
