#pragma once

/// This header provides a logging implements which does not generate any output

namespace SomeIP_utils {

struct LogData {

	template<typename T>
	LogData& operator<<(const T& v) {
		return *this;
	}

};

struct LogMainLoopIntegration {
};

#ifndef log_debug
#define log_debug(args ...) SomeIP_utils::LogData()
#define log_verbose(args ...) SomeIP_utils::LogData()
#define log_error(args ...) SomeIP_utils::LogData()
#define log_info(args ...) SomeIP_utils::LogData()
#define log_warning(args ...) SomeIP_utils::LogData()

#define LOG_DEFINE_APP_IDS(appID, appDescription)
#define LOG_DECLARE_CONTEXT(contextName, contextShortID, contextDescription)
#define LOG_IMPORT_CONTEXT(contextName)
#define LOG_SET_DEFAULT_CONTEXT(context)
#define LOG_SET_CLASS_CONTEXT(context)
#define LOG_DECLARE_DEFAULT_CONTEXT(context, contextID, contextDescription)
#define LOG_DECLARE_DEFAULT_LOCAL_CONTEXT(contextShortID, contextDescription)
#define LOG_IMPORT_DEFAULT_CONTEXT(context)
#define LOG_DECLARE_CLASS_CONTEXT(contextShortID, contextDescription)
#define LOG_FILE_SET_OUTPUT(path)
#endif

}
