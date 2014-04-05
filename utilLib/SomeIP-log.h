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
		Input<pelagicore::ConsoleLogContext, pelagicore::DltContextClass, SomeIPFileLoggingContext>,
		Output<pelagicore::ConsoleLogContext::LogDataType,
				pelagicore::DltContextClass::LogDataType, SomeIPFileLoggingContext::LogDataType> > LogContext;

typedef pelagicore::LogContextT<Input<pelagicore::DltContextClass>, Output<DltLogData> > LogContext9;
typedef pelagicore::LogContextT<Input<pelagicore::DltContextClass,
				      pelagicore::DltContextClass>, Output<DltLogData, DltLogData> > LogContext8;
typedef pelagicore::LogContextT<Input<pelagicore::ConsoleLogContext
				      >, Output<ConsoleLogData> > LogContext2;

}

#include "log-types.h"

#include "SomeIP-Config.h"
