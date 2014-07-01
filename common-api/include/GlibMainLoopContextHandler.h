#pragma once

#include <CommonAPI/CommonAPI.h>
#include <memory>
#include <map>

#include <glib.h>

namespace CommonAPI {

/**
 * That class can be used to
 */
class GlibMainLoopContextHandler {

public:
	GlibMainLoopContextHandler(std::shared_ptr<MainLoopContext> mainLoopContext) :
		m_mainLoopContext(mainLoopContext) {
		m_mainContext = g_main_context_new();
		init();
	}

	GlibMainLoopContextHandler(std::shared_ptr<MainLoopContext> mainLoopContext, GMainContext* mainContext) :
		m_mainLoopContext(mainLoopContext) {
		m_mainContext = mainContext;
		init();
	}

	~GlibMainLoopContextHandler() {
		if (m_mainContext != nullptr)
			g_main_context_unref(m_mainContext);
	}

	void runMainLoop() {
		GMainLoop* mainLoop = g_main_loop_new(m_mainContext, FALSE);
		g_main_loop_run(mainLoop);
		g_main_loop_unref(mainLoop);
	}

private:
	void init() {
		doSubscriptions();
	}

	static gboolean gTimeoutDispatcher(void* userData) {
		return static_cast<CommonAPI::DispatchSource*>(userData)->dispatch();
	}

	void timeoutAddedCallback(CommonAPI::Timeout* commonApiTimeoutSource,
				  const CommonAPI::DispatchPriority dispatchPriority) {
		GSource* gTimeoutSource = g_timeout_source_new( commonApiTimeoutSource->getTimeoutInterval() );
		g_source_set_callback(gTimeoutSource, &GlibMainLoopContextHandler::gTimeoutDispatcher, commonApiTimeoutSource,
				      NULL);
		g_source_attach(gTimeoutSource, m_mainContext);
	}

	void timeoutRemovedCallback(CommonAPI::Timeout* timeout) {
		g_source_remove_by_user_data(timeout);
	}

	void watchRemovedCallback(CommonAPI::Watch* watch) {
		// TODO : implement
		//		assert(false);

		//		g_source_remove_by_user_data(watch);
		//
		//		if (dbusChannel_) {
		//			g_io_channel_unref(dbusChannel_);
		//			dbusChannel_ = NULL;
		//		}
		//
		//		const auto& dependentSources = watch->getDependentDispatchSources();
		//		for (auto dependentSourceIterator = dependentSources.begin(); dependentSourceIterator != dependentSources.end();
		//		     dependentSourceIterator++) {
		//			GSource* gDispatchSource = g_source_new( &standardGLibSourceCallbackFuncs, sizeof(GDispatchWrapper) );
		//			GSource* gSource = gSourceMappings.find(*dependentSourceIterator)->second;
		//			g_source_destroy(gSource);
		//			g_source_unref(gSource);
		//		}
	}

	static gboolean gWatchDispatcher(GIOChannel* source, GIOCondition condition, gpointer userData) {
		CommonAPI::Watch* watch = static_cast<CommonAPI::Watch*>(userData);
		watch->dispatch(condition);
		return true;
	}

	class GDispatchWrapper : public GSource {
public:
		GDispatchWrapper(CommonAPI::DispatchSource* dispatchSource) :
			m_dispatchSource(dispatchSource) {
		}
		CommonAPI::DispatchSource* m_dispatchSource;
	};

	static gboolean dispatchPrepare(GSource* source, gint* timeout) {
		int64_t eventTimeout;
		return static_cast<GDispatchWrapper*>(source)->m_dispatchSource->prepare(eventTimeout);
	}

	static gboolean dispatchCheck(GSource* source) {
		return static_cast<GDispatchWrapper*>(source)->m_dispatchSource->check();
	}

	static gboolean dispatchExecute(GSource* source, GSourceFunc callback, gpointer userData) {
		static_cast<GDispatchWrapper*>(source)->m_dispatchSource->dispatch();
		return true;
	}

	static GSourceFuncs& getStandardGLibSourceCallbackFuncs() {
		static GSourceFuncs standardGLibSourceCallbackFuncs = {dispatchPrepare, dispatchCheck, dispatchExecute, nullptr};
		return standardGLibSourceCallbackFuncs;
	}

	void watchAddedCallback(CommonAPI::Watch* watch, const CommonAPI::DispatchPriority dispatchPriority) {
		const pollfd& fileDesc = watch->getAssociatedFileDescriptor();
		GIOChannel* ioChannel = g_io_channel_unix_new(fileDesc.fd);

		GSource* gWatch = g_io_create_watch( ioChannel, static_cast<GIOCondition>(fileDesc.events) );
		g_source_set_callback(gWatch, reinterpret_cast<GSourceFunc>(&gWatchDispatcher), watch, NULL);

		//		const auto& dependentSources = watch->getDependentDispatchSources();
		for ( auto& dependentSource : watch->getDependentDispatchSources() ) {
			GSource* gDispatchSource = g_source_new( &getStandardGLibSourceCallbackFuncs(), sizeof(GDispatchWrapper) );
			static_cast<GDispatchWrapper*>(gDispatchSource)->m_dispatchSource = dependentSource;

			g_source_add_child_source(gWatch, gDispatchSource);

			gSourceMappings.insert({dependentSource, gDispatchSource});
		}
		g_source_attach(gWatch, m_mainContext);
	}

	void wakeupMain() {
		g_main_context_wakeup(m_mainContext);
	}

	void doSubscriptions() {
		m_mainLoopContext->subscribeForTimeouts(
			std::bind(&GlibMainLoopContextHandler::timeoutAddedCallback, this, std::placeholders::_1,
				  std::placeholders::_2),
			std::bind(&GlibMainLoopContextHandler::timeoutRemovedCallback, this, std::placeholders::_1) );

		m_mainLoopContext->subscribeForWatches(
			std::bind(&GlibMainLoopContextHandler::watchAddedCallback, this, std::placeholders::_1,
				  std::placeholders::_2),
			std::bind(&GlibMainLoopContextHandler::watchRemovedCallback, this, std::placeholders::_1) );

		m_mainLoopContext->subscribeForWakeupEvents( std::bind(&GlibMainLoopContextHandler::wakeupMain, this) );
	}

	std::shared_ptr<MainLoopContext> m_mainLoopContext;
	std::map<CommonAPI::DispatchSource*, GSource*> gSourceMappings;

	GMainContext* m_mainContext;

};

}
