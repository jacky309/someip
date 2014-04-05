#pragma once

// Enable verbose logs for the console output, which are disabled by default
//#define LOG_CONSOLE_SEVERITY LogLevel::Verbose

#include "plog.h"

#ifdef ENABLE_DLT_LOGGING
#include "log-dlt.h"
#endif

#ifdef ENABLE_CONSOLE_LOGGING
#include "log-console.h"
#endif

#include "log-file.h"

class SomeIPFileLoggingContext : public pelagicore::FileLogContext {
public:

	FILE* getFile() override {
//		printf("%i\n", m_file);
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

namespace pelagicore {

typedef pelagicore::LogContextT<
		Input<pelagicore::ConsoleLogContext
#ifdef ENABLE_DLT_LOGGING
		, pelagicore::DltContextClass
#endif
		, SomeIPFileLoggingContext>,
		Output<pelagicore::ConsoleLogContext::LogDataType
#ifdef ENABLE_DLT_LOGGING
		, 
		pelagicore::DltContextClass::LogDataType
#endif
		, SomeIPFileLoggingContext::LogDataType> > LogContext;

}

#include "log-types.h"

#include "SomeIP-Config.h"
