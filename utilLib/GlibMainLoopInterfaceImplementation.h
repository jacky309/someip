#pragma once

#include "GlibIO.h"

namespace SomeIP_utils {

using namespace SomeIP_Lib;

/**
 * That class implements the MainLoopInterface using glib's main loop functions
 */
class GlibMainLoopInterfaceImplementation : public MainLoopInterface {
public:
	GlibMainLoopInterfaceImplementation(GMainContext* context = nullptr) : m_context(context) {
	}

	class GLibIdle : public IdleMainLoopHook {

public:
		GLibIdle(CallBackFunction callBackFunction, GMainContext* context) : m_glibIdle(callBackFunction, context) {
		}

		void activate() override {
			m_glibIdle.activate();
		}

private:
		GlibIdleCallback m_glibIdle;
	};

	class GLibTimeOut : public TimeOutMainLoopHook {
public:
		GLibTimeOut(CallBackFunction callBackFunction, int durationInMilliseconds, GMainContext* context) :
			m_timer(callBackFunction, durationInMilliseconds, context) {
			m_callBack = callBackFunction;
		}

		void activate() override {
			assert(false);
		}

private:
		CallBackFunction m_callBack;
		GLibTimer m_timer;
	};

	class GLibFileDescriptorWatch : public WatchMainLoopHook {
public:
		GLibFileDescriptorWatch(CallBackFunction callBackFunction, const pollfd& fd,
					GMainContext* context) : m_mainContext(context), m_fd(fd) {
			m_callBack = callBackFunction;
			m_channel = g_io_channel_unix_new(fd.fd);
		}

public:
		~GLibFileDescriptorWatch() {
			disable();
			if (m_channel != nullptr)
				g_io_channel_unref(m_channel);
		}

		void disable() override {
			if (inputSourceID != UNREGISTERED_SOURCE)
				g_source_remove(inputSourceID);
			inputSourceID = UNREGISTERED_SOURCE;
		}

		void enable() override {
			if (!m_isInputWatched) {

				GIOCondition condition = static_cast<GIOCondition>(0);
				if (m_fd.events & POLLIN)
					condition |= G_IO_IN;
				if (m_fd.events & POLLHUP)
					condition |= G_IO_HUP;

				inputSourceID =
					g_io_add_watch_full_with_context(m_channel, G_PRIORITY_DEFAULT, condition,
									 onSocketDataAvailableGlibCallback, this, NULL,
									 m_mainContext);
				m_isInputWatched = true;
			}
		}

		static gboolean onSocketDataAvailableGlibCallback(GIOChannel* gio, GIOCondition condition, gpointer data) {
			GLibFileDescriptorWatch* client = static_cast<GLibFileDescriptorWatch*>(data);
			client->m_callBack();
			return true;
		}

		/**
		 * g_io_add_watch_full:
		 *
		 * @param channel a GIOChannel
		 * @param priority the priority of the GIOChannel source
		 * @param condition the condition to watch for
		 * @param func the function to call when the condition is satisfied
		 * @param user_data user data to pass to @func
		 * @param notify the function to call when the source is removed
		 * @param context the GMainContext to use
		 *
		 * Adds the GIOChannel into the given main loop context
		 * with the given priority.
		 *
		 * This internally creates a main loop source using g_io_create_watch()
		 * and attaches it to the main loop context with g_source_attach().
		 * You can do these steps manually if you need greater control.
		 *
		 * @return the event source id
		 */
		static guint g_io_add_watch_full_with_context(GIOChannel* channel, gint priority, GIOCondition condition,
							      GIOFunc func,
							      gpointer user_data, GDestroyNotify notify,
							      GMainContext* context) {
			GSource* source;
			guint id;

			g_return_val_if_fail(channel != NULL, 0);
			source = g_io_create_watch(channel, condition);

			if (priority != G_PRIORITY_DEFAULT)
				g_source_set_priority(source, priority);
			g_source_set_callback(source, (GSourceFunc) func, user_data, notify);

			id = g_source_attach(source, context);
			g_source_unref(source);

			return id;
		}

private:
		bool m_isInputWatched = false;
		gint inputSourceID = UNREGISTERED_SOURCE;

		GIOChannel* m_channel = nullptr;
		GMainContext* m_mainContext;
		pollfd m_fd;

private:
		CallBackFunction m_callBack;
	};

	std::unique_ptr<IdleMainLoopHook> addIdle(IdleMainLoopHook::CallBackFunction callBackFunction) override {
		return std::unique_ptr<IdleMainLoopHook>( new GLibIdle(callBackFunction, m_context) );
	}

	std::unique_ptr<TimeOutMainLoopHook> addTimeout(TimeOutMainLoopHook::CallBackFunction callBackFunction,
							int durationInMilliseconds) override {
		return std::unique_ptr<TimeOutMainLoopHook>( new GLibTimeOut(callBackFunction, durationInMilliseconds, m_context) );
	}

	std::unique_ptr<WatchMainLoopHook> addFileDescriptorWatch(WatchMainLoopHook::CallBackFunction callBackFunction,
						    const pollfd& fd) override {
		return std::unique_ptr<WatchMainLoopHook>( new GLibFileDescriptorWatch(callBackFunction, fd, m_context) );
	}

private:
	GMainContext* m_context;

};

}
