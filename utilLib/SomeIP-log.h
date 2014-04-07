#pragma once

// Enable verbose logs for the console output, which are disabled by default
//#define LOG_CONSOLE_SEVERITY LogLevel::Verbose

#include "plog.h"

#ifdef ENABLE_DLT_LOGGING
#include "log-dlt.h"
#endif

#include "log-console.h"
#include "log-file.h"

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
