#pragma once

#include "ivi-logging.h"

#ifdef ENABLE_DLT_LOGGING
#include "log-dlt.h"
#endif

#include "log-console.h"
#include "log-file.h"
#include "log-null.h"

#define ENABLE_TRAFFIC_LOGGING

#ifdef ENABLE_TRAFFIC_LOGGING
#define log_traffic log_debug
#else
#define log_traffic(args ...) while (false) NullLogData()
#endif

class SomeIPFileLoggingContext : public logging::FileLogContext {

public:
	SomeIPFileLoggingContext() {
		setLogLevel(logging::LogLevel::Info);
	}

	FILE* getFile(logging::ConsoleLogData& data) override {
		return m_file;
	}

	static void openFile(const char* fileName) {
		if (m_file == nullptr) {
			m_file = fopen(fileName, "w");
		}
		assert(m_file != nullptr);
	}

	static FILE* m_file;
};

/**
 * We set our logging "LogContext" typedef to log to the console, the log file, and the DLT if available
 */
typedef logging::LogContextT<
	logging::TypeSet<logging::ConsoleLogContext
#ifdef ENABLE_DLT_LOGGING
			 , logging::DltContextClass
#endif
			 , SomeIPFileLoggingContext>,
	logging::TypeSet<logging::ConsoleLogContext::LogDataType
#ifdef ENABLE_DLT_LOGGING
			 , logging::DltContextClass::LogDataType
#endif
			 , SomeIPFileLoggingContext::LogDataType> > LogContext;

#include "SomeIP-Config.h"
