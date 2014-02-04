#pragma once

#include <glib.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

namespace SomeIP_utils {

/**
 * This class helps you parsing command line parameters.
 * The standard "--version" command line parameter is handled automatically and a string containing the value of
 */
class CommandLineParser {
public:
	CommandLineParser(const char* summary, const char* param = "param1 param2", const char* version = "undefined",
			  const char* description = "") {
		context = g_option_context_new(param);
		g_option_context_set_summary(context, summary);
		g_option_context_set_description(context, description);

		m_version = version;

		addArgument(bPrintVersion, "version", 0, "Print Version");
	}

	template<typename T> const char* defaultString(const char* argumentDescription, T value) {
		std::ostringstream defaultValue;
		if (argumentDescription != NULL)
			defaultValue << argumentDescription;
		defaultValue << " (Default: " << value << ")";
		m_defaultValueStrings.push_back( std::string( defaultValue.str() ) );
		return m_defaultValueStrings[m_defaultValueStrings.size() - 1].c_str();
	}

	struct BoolTuple {

		BoolTuple(bool& p) :
			theBool(p) {
			valueAsGBool = p;
		}

		void assign() {
			theBool = valueAsGBool;
		}

		gboolean valueAsGBool;
		bool& theBool;
	};

	/**
	 * Add a bool parameter to the list of command line options to handle
	 */
	GOptionEntry& addArgument(bool& value, const char* longName, char shortName, const char* description,
				  const char* argumentDescription = NULL) {

		m_boolEntries.push_back( new BoolTuple(value) );

		auto& tuple = m_boolEntries.at(m_boolEntries.size() - 1);

		GOptionEntry entry = {longName, shortName, 0, G_OPTION_ARG_NONE, &tuple->valueAsGBool, defaultString(description,
														     value),
				      argumentDescription};
		m_entries.push_back(entry);

		return m_entries.back();
	}

	/**
	 * Add an int parameter to the list of command line options to handle
	 */
	GOptionEntry& addArgument(int& value, const char* longName, char shortName, const char* description,
				  const char* argumentDescription = NULL) {
		GOptionEntry entry = {longName, shortName, 0, G_OPTION_ARG_INT, &value, defaultString(description, value),
				      argumentDescription};
		m_entries.push_back(entry);
		return m_entries.back();
	}

	/**
	 * Add an int parameter to the list of command line options to handle
	 */
	GOptionEntry& addArgument(unsigned int& value, const char* longName, char shortName, const char* description,
				  const char* argumentDescription = NULL) {
		GOptionEntry entry = {longName, shortName, 0, G_OPTION_ARG_INT, &value, defaultString(description, value),
				      argumentDescription};
		m_entries.push_back(entry);
		return m_entries.back();
	}

	/**
	 * Add a string parameter to the list of command line options to handle
	 */
	GOptionEntry& addArgument(const char*& value, const char* longName, char shortName, const char* description,
				  const char* argumentDescription = NULL) {
		GOptionEntry entry = {longName, shortName, 0, G_OPTION_ARG_STRING, &value, defaultString(description, value),
				      argumentDescription};
		m_entries.push_back(entry);
		return m_entries.back();
	}

	/**
	 * Parse the given command line. When this method returns, the variables which where passed to addArgument() calls should be set.
	 */
	bool parse(int& argc, const char**& argv) {

		GOptionEntry entry = {NULL};

		m_entries.push_back(entry);

		g_option_context_add_main_entries(context, &(m_entries[0]), NULL);

		GError* error = NULL;
		if ( !g_option_context_parse(context, &argc, (gchar***) &argv, &error) ) {
			fprintf(stderr, "%s\n", error->message);
			return true;
		}

		for (auto& tuple : m_boolEntries)
			tuple->assign();

		if (bPrintVersion) {
			printf("Package version: '%s'\n", m_version);
			return true;
		}

		return false;
	}

	bool parse(int& argc, char**& argv) {
		return parse(argc, (const char** &)argv);
	}

	void printHelp() {
		gchar* helpString = g_option_context_get_help(context, true, NULL);
		fprintf(stderr, "%s\n", helpString);
	}

private:
	bool bPrintVersion = false;

	const char* m_version;

	std::vector<BoolTuple*> m_boolEntries;
	std::vector<std::string> m_defaultValueStrings;

	std::vector<GOptionEntry> m_entries;

	GOptionContext* context;
};

}
