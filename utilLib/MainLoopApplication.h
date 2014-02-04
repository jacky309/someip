#pragma once

#include "SomeIP-common.h"

#include "glib.h"
#include <unistd.h>
#include <stdlib.h>

#ifdef __linux__
#include <sys/signalfd.h>
#else

class MainLoopApplication;
static MainLoopApplication* s_appInstance = NULL;
#endif

namespace SomeIP_utils {

/**
 * This class provides the following features:
 * \li Setup signal handlers to properly terminate the application
 * \li A run() method, which runs a Glib main loop until a given timeout is reached
 * \li Integration of logging back channel handling into the main loop
 */
class MainLoopApplication {
public:
	MainLoopApplication() {
	}

	~MainLoopApplication() {
		if (m_mainLoop != nullptr)
			g_main_loop_unref(m_mainLoop);
	}

	static gboolean timeout_callback(gpointer data) {
		auto app = static_cast<MainLoopApplication*>(data);
		app->exit();
		return FALSE;
	}

	/**
	 * Run the main loop until either either the given timeout is reached, or a termination signal is received
	 */
	void run(unsigned int timeoutInMilliseconds = 0) {
		setupSignalHandling();

		m_mainLoop = g_main_loop_new(getMainContext(), FALSE);

		if (timeoutInMilliseconds != 0)
			g_timeout_add(timeoutInMilliseconds, timeout_callback, this);

		log_info("Entering main loop");

		g_main_loop_run(m_mainLoop);
	}

	void exit() {
		log_info("Exiting main loop");
		g_main_loop_quit(m_mainLoop);
	}

	GMainContext* getMainContext() {
		return NULL;
	}

	/**
	 *  Signal handler
	 */
	void processUnixSignal(int sig) {
		log_info("Signal received");
		switch (sig) {
		case SIGABRT :
			log_warning("Received SIGABRT");
			break;
		case SIGPIPE :
			log_warning("Received SIGPIPE. Client disconnected");
			break;
		default :
			log_warning("Initiating shutdown procedure after receiving signal %i...", sig);
			exit();
			break;
		}
	}

	/**
	 * Return
	 */
	bool isSigIntHandlingEnabled() {
		// If we run in a debugger, we need to work around the problem that GDB forwards the SIGINT to our application
		// when it is asked to interrupt the execution, which causes our application to shutdown
		return (getenv("EclipseVersion") != nullptr);
	}

#ifdef __linux__

	int unixSignalFileDescriptor;

	gboolean onUnixSignalReceived(GIOChannel* channel, GIOCondition condition) {
		struct signalfd_siginfo fdsi;

		int s = read( unixSignalFileDescriptor, &fdsi, sizeof(struct signalfd_siginfo) );
		if ( s != sizeof(struct signalfd_siginfo) )
			log_error("Error reading signal value");

		processUnixSignal(fdsi.ssi_signo);
		return true;
	}

	static gboolean onUnixSignalReceived(GIOChannel* channel, GIOCondition condition, gpointer data) {
		return static_cast<MainLoopApplication*>(data)->onUnixSignalReceived(channel, condition);
	}

#else

	static void onSignalReceived(int signal) {
		s_appInstance->exit();
	}

#endif

	static void doNothing(int signal) {
	}

	void setupSignalHandling() {

#ifdef __linux__
		/* catch Ctrl-C and Hangup signals for clean shutdown */
		sigset_t mask;
		sigemptyset(&mask);
		//		sigaddset(&mask, SIGINT);
		sigaddset(&mask, SIGHUP);
		sigaddset(&mask, SIGTERM);
		sigaddset(&mask, SIGPIPE);
		sigaddset(&mask, SIGQUIT);
		sigaddset(&mask, SIGKILL);
		sigaddset(&mask, SIGABRT);

		if ( !isSigIntHandlingEnabled() )
			sigaddset(&mask, SIGINT);

		if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
			log_error("Error during sigprocmask() call");

		unixSignalFileDescriptor = signalfd(-1, &mask, 0);
		if (unixSignalFileDescriptor == -1)
			log_error("Error during signalfd() call");

		GIOChannel* channel = g_io_channel_unix_new(unixSignalFileDescriptor);
		g_io_add_watch(channel, (GIOCondition)(G_IO_IN | G_IO_HUP), onUnixSignalReceived, this);
#else
		s_appInstance = this;

		//		signal(SIGINT, onSignalReceived);
		//		signal(SIGHUP, onSignalReceived);
		signal(SIGTERM, onSignalReceived);
		signal(SIGPIPE, doNothing);
		signal(SIGQUIT, onSignalReceived);
		signal(SIGKILL, onSignalReceived);
		signal(SIGABRT, onSignalReceived);

		if ( !isSigIntHandlingEnabled() )
			signal(SIGINT, onSignalReceived);

#endif

	}

	GMainLoop* m_mainLoop = NULL;
	LogMainLoopIntegration m_logMainLoopIntegration;
};

}
