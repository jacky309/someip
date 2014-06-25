#pragma once

#include "glib.h"
#include <functional>

namespace SomeIP_utils {

inline GIOCondition operator|(const GIOCondition c1, const GIOCondition c2) {
	return static_cast<GIOCondition>( static_cast<int>(c1) | static_cast<int>(c2) );
}

class GLibTimer {

	std::function<void(void)> m_func;

public:
	GLibTimer(std::function<void(void)> func, int duration, GMainContext* mainContext) :
		m_func(func) {
		m_mainContext = mainContext;
		setDuration(duration);
		start();
	}

	~GLibTimer() {
		stop();
	}

	void setDuration(int duration) {
		m_duration = duration;
	}

	void start() {
		m_time = 0;
		m_source = g_timeout_source_new(m_duration);
		g_source_set_callback(m_source, onTimerCallback, this, nullptr);
		g_source_attach(m_source, m_mainContext);
	}

	void stop() {
		if (m_source != nullptr) {
			g_source_destroy(m_source);
			g_source_unref(m_source);
			m_source = nullptr;
		}
	}

	/**
	 * Return an inaccurate approximation of the elapsed time since the timer has been started
	 */
	int getTime() const {
		return m_time;
	}

private:
	void onTimerCallback() {
		m_time += m_duration;
		m_func();
	}

	static gboolean onTimerCallback(gpointer data) {
		GLibTimer& timer = *( (GLibTimer*) data );
		timer.onTimerCallback();

		// return true so that the timer keeps going
		return true;
	}

	GMainContext* m_mainContext;
	GSource* m_source = nullptr;
	int m_duration;
	int m_time;
};

static const int UNREGISTERED_SOURCE = -1;

class GlibIdleCallback {

public:
	typedef std::function<bool (void)> CallbackFunction;

	GlibIdleCallback(CallbackFunction func, GMainContext* mainContext) :
		m_func(func) {
		m_mainContext = mainContext;
	}

	~GlibIdleCallback() {
		destroySource();
	}

	void activate() {
		if (m_source == nullptr) {
			m_source = g_idle_source_new();
			g_source_set_callback(m_source, &onGlibCallback, this, nullptr);
			g_source_attach(m_source, m_mainContext);
		}
	}

private:
	void destroySource() {
		if (m_source != nullptr) {
			g_source_destroy(m_source);
			g_source_unref(m_source);
			m_source = nullptr;
		}
	}

	static gboolean onGlibCallback(gpointer data) {

		GlibIdleCallback& timer = *static_cast<GlibIdleCallback*>(data);

		if ( timer.m_func() )
			return TRUE;
		else {
			timer.destroySource();
			return FALSE;
		}

	}

	GSource* m_source = nullptr;
	GMainContext* m_mainContext = nullptr;
	CallbackFunction m_func;
};

enum class WatchStatus {
	KEEP_WATCHING, STOP_WATCHING
};

struct GlibChannelListener {
	virtual ~GlibChannelListener() {
	}
	virtual WatchStatus onIncomingDataAvailable() = 0;
	virtual void onDisconnected() = 0;
	virtual WatchStatus onWritingPossible() = 0;
};

class GlibChannelWatcher {

public:
	GlibChannelWatcher(GlibChannelListener& listener, GMainContext* mainContext) :
		m_mainContext(mainContext), m_listener(listener) {
	}

	~GlibChannelWatcher() {
		disableWatch();
		if (m_channel != nullptr)
			g_io_channel_unref(m_channel);
	}

	void setup(int fileDescriptor) {
		m_channel = g_io_channel_unix_new(fileDescriptor);
	}

	void disableInputWatch() {
		if (inputSourceID != UNREGISTERED_SOURCE)
			g_source_remove(inputSourceID);
		inputSourceID = UNREGISTERED_SOURCE;
	}

	void disableOutputWatch() {
		if (outputSourceID != UNREGISTERED_SOURCE)
			g_source_remove(outputSourceID);
		outputSourceID = UNREGISTERED_SOURCE;
	}

	void disableWatch() {
		disableInputWatch();
		disableOutputWatch();
	}

	void enableOutputWatch() {
		if (outputSourceID == UNREGISTERED_SOURCE)
			outputSourceID = g_io_add_watch_full_with_context(m_channel, G_PRIORITY_DEFAULT, G_IO_OUT,
									  onWritingPossibleGlibCallback, this, NULL,
									  m_mainContext);
	}

	void enableInputWatch() {
		if (!m_isInputWatched) {
			inputSourceID =
				g_io_add_watch_full_with_context(m_channel, G_PRIORITY_DEFAULT, G_IO_IN | G_IO_HUP,
								 onSocketDataAvailableGlibCallback, this, NULL,
								 m_mainContext);
			m_isInputWatched = true;
		}
	}

	void enableWatch() {
		enableInputWatch();

	}

	static gboolean onWritingPossibleGlibCallback(GIOChannel* gio, GIOCondition condition, gpointer data) {
		GlibChannelWatcher* client = static_cast<GlibChannelWatcher*>(data);

		auto v = (client->m_listener.onWritingPossible() == WatchStatus::KEEP_WATCHING);
		if (!v)
			client->outputSourceID = UNREGISTERED_SOURCE;

		return v;
	}

	static gboolean onSocketDataAvailableGlibCallback(GIOChannel* gio, GIOCondition condition, gpointer data) {
		GlibChannelWatcher* client = static_cast<GlibChannelWatcher*>(data);

		if (condition & G_IO_HUP) {
			client->m_listener.onDisconnected();
			return false;
		}

		client->m_listener.onIncomingDataAvailable();

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
	static guint g_io_add_watch_full_with_context(GIOChannel* channel, gint priority, GIOCondition condition, GIOFunc func,
						      gpointer user_data, GDestroyNotify notify, GMainContext* context) {
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
	//	bool isOutputWatched = false;
	gint inputSourceID = UNREGISTERED_SOURCE;
	gint outputSourceID = UNREGISTERED_SOURCE;

	GIOChannel* m_channel = nullptr;
	GMainContext* m_mainContext;
	GlibChannelListener& m_listener;
};

}
