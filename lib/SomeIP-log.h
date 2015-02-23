#pragma once

#include "ivi-logging.h"
#include "ivi-logging-file.h"
#include "ivi-logging-null.h"

#define ENABLE_TRAFFIC_LOGGING

#ifdef ENABLE_TRAFFIC_LOGGING
#define log_traffic log_debug
#else
#define log_traffic(args ...) while (false) NullLogData()
#endif

namespace SomeIP_utils {

class SomeIPFileLoggingContext : public logging::FileLogContext {

public:
	SomeIPFileLoggingContext() {
		setLogLevel(logging::LogLevel::Info);
	}

};

// We reuse the default configuration of ivi-logging, but we add our own file logging backend
typedef logging::DefaultLogContext::Extension<SomeIPFileLoggingContext, SomeIPFileLoggingContext::LogDataType> LogContext;

}

#include "SomeIP-Config.h"
