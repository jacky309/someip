#pragma once

#include "SomeIP-clientLib.h"
#include "CommonAPI-SomeIP.h"
#include <unordered_map>

#include <CommonAPI/ServicePublisher.h>

namespace CommonAPI {
namespace SomeIP {

using namespace::SomeIP;

class SomeIPStubAdapter;
class SomeIPProxy;

class SomeIPConnection : private SomeIPClient::ClientConnectionListener, public CommonAPI::ServicePublisher,
	public MainLoopInterface {
	LOG_SET_CLASS_CONTEXT(someIPCommonAPILogContext);

	class SomeIPConnectionWatch;

	class SomeIPDispatchSource : public DispatchSource {
public:
		SomeIPDispatchSource(SomeIPClient::ClientConnection& connection) :
			m_connection(connection) {
		}

		bool check() {
			// TODO : optimize that function, since we seem to be called very frequently with a QML UI
			return m_connection.hasIncomingMessages();
		}

		bool dispatch() {
			m_connection.dispatchIncomingMessages();
			m_prepared = false;
			return false;
		}

		bool prepare(int64_t& timeout) {
			timeout = 0;
			m_prepared = true;
			return false;
		}

private:
		SomeIPClient::ClientConnection& m_connection;
		bool m_prepared = true;

	};

	class SomeIPConnectionWatch : public CommonAPI::Watch {

		SomeIPConnectionWatch(SomeIPClient::ClientConnection& connection) :
			m_connection(connection), m_dispatchSource(connection) {
			m_dependentDispatchSources.push_back(&m_dispatchSource);
		}

		/**
		 * \brief Dispatches the watch.
		 *
		 * Should only be called once the associated file descriptor has events ready.
		 *
		 * @param eventFlags The events that shall be retrieved from the file descriptor.
		 */
		void dispatch(unsigned int eventFlags) override {
			m_connection.dispatchIncomingMessages();
		}

		/**
		 * \brief Returns the file descriptor that is managed by this watch.
		 *
		 * @return The associated file descriptor.
		 */
		const pollfd& getAssociatedFileDescriptor() override {
			m_pollfd.events = POLLERR | POLLHUP | POLLIN;
			m_pollfd.revents = 0;
			m_pollfd.fd = m_connection.getFileDescriptor();
			return m_pollfd;
		}

		/**
		 * \brief Returns a vector of all dispatch sources that depend on the watched file descriptor.
		 *
		 * The returned vector will not be empty if and only if there are any sources
		 * that depend on availability of data of the watched file descriptor. Whenever this
		 * Watch is dispatched, those sources likely also need to be dispatched.
		 */
		const std::vector<DispatchSource*>& getDependentDispatchSources() override {
			return m_dependentDispatchSources;
		}

private:
		pollfd m_pollfd;
		std::vector<DispatchSource*> m_dependentDispatchSources;
		SomeIPClient::ClientConnection& m_connection;
		SomeIPDispatchSource m_dispatchSource;

		friend class SomeIPConnection;
	};

	class CommonAPIIdleMainLoopHook : public IdleMainLoopHook {

public:
		CommonAPIIdleMainLoopHook(CallBackFunction callBackFunction, std::shared_ptr<MainLoopContext> mainLoopContext) {
			m_callBack = callBackFunction;
		}

		void activate() override {
			// TODO
			assert(false);
		}

private:
		CallBackFunction m_callBack;
		std::shared_ptr<MainLoopContext> m_mainLoopContext;
	};

	class CommonAPITimeOutMainLoopHook : public TimeOutMainLoopHook {
public:
		CommonAPITimeOutMainLoopHook(CallBackFunction callBackFunction, int durationInMilliseconds,
					     std::shared_ptr<MainLoopContext> mainLoopContext) {
			m_callBack = callBackFunction;
		}

private:
		CallBackFunction m_callBack;
		std::shared_ptr<MainLoopContext> m_mainLoopContext;
	};

	class CommonAPIWatchMainLoopHook : public WatchMainLoopHook, private Watch {
public:
		CommonAPIWatchMainLoopHook(CallBackFunction callBackFunction, struct pollfd& fd,
					   std::shared_ptr<MainLoopContext> mainLoopContext) :
			m_mainLoopContext(mainLoopContext), m_fd(fd) {
			m_callBack = callBackFunction;
		}

		~CommonAPIWatchMainLoopHook() {
			disable();
			//			assert(false);
		}

		void enable() override {
			m_mainLoopContext->registerWatch(this);
		}

		void dispatch(unsigned int eventFlags) override {
			m_callBack();
		}

		const pollfd& getAssociatedFileDescriptor() {
			return m_fd;
		}

		const std::vector<DispatchSource*>& getDependentDispatchSources() {
			return m_dependentSources;
		}

		void disable() override {
			m_mainLoopContext->deregisterWatch(this);
		}

private:
		CallBackFunction m_callBack;
		std::shared_ptr<MainLoopContext> m_mainLoopContext;
		struct pollfd& m_fd;
		std::vector<DispatchSource*> m_dependentSources;
	};

public:
	SomeIPConnection(SomeIPClient::ClientConnection* clientConnection) : m_connection(clientConnection),
		m_watch( getConnection() ) {
	}

	~SomeIPConnection() {
		getConnection().disconnect();
	}

	std::unique_ptr<IdleMainLoopHook> addIdleCallback(IdleMainLoopHook::CallBackFunction callBackFunction) override {
		return std::unique_ptr<IdleMainLoopHook>( new CommonAPIIdleMainLoopHook(callBackFunction, m_mainLoopContext) );
	}

	std::unique_ptr<TimeOutMainLoopHook> addTimeout(TimeOutMainLoopHook::CallBackFunction callBackFunction,
							int durationInMilliseconds) override {
		return std::unique_ptr<TimeOutMainLoopHook>( new CommonAPITimeOutMainLoopHook(callBackFunction,
											      durationInMilliseconds,
											      m_mainLoopContext) );
	}

	std::unique_ptr<WatchMainLoopHook> addWatch(WatchMainLoopHook::CallBackFunction callBackFunction, pollfd& fd) override {
		return std::unique_ptr<WatchMainLoopHook>( new CommonAPIWatchMainLoopHook(callBackFunction, fd, m_mainLoopContext) );
	}

	Watch& getWatch() {
		return m_watch;
	}

	SomeIPReturnCode connect(std::shared_ptr<MainLoopContext> mainLoopContext) {
		m_mainLoopContext = mainLoopContext;
		getConnection().setMainLoopInterface(*this);
		return getConnection().connect(*this);
	}

	bool isConnected() const {
		return getConnection().isConnected();
	}

	void onDisconnected() {
		log_info();
	}

	SomeIPReturnCode sendMessage(OutputMessage& msg) {
		return getConnection().sendMessage(msg);
	}

	InputMessage sendMessageBlocking(OutputMessage& msg) {
		return getConnection().sendMessageBlocking(msg);
	}

	void sendWithResponseHandler(OutputMessage& msg,
				     MessageSink& messageHandler) {
		m_messageHandlers[msg.getHeader().getRequestID()] = &messageHandler;
		sendMessage(msg);
	}

	SomeIPReturnCode registerService(SomeIPStubAdapter& service);

	SomeIPReturnCode registerProxy(SomeIPProxy& proxy);

	void unregisterService(ServiceID serviceID) {
		getConnection().unregisterService(serviceID);
	}

	//	void unregisterProxy(ServiceID serviceID) {
	//		getConnection().unregisterProxy(serviceID);
	//	}

	void subscribeNotification(MessageID messageID) {
		getConnection().subscribeToNotifications(messageID);
	}

	MessageProcessingResult processMessage(const InputMessage& message);

	SomeIPClient::ClientConnection& getConnection() {
		return *m_connection.get();
	}

	const SomeIPClient::ClientConnection& getConnection() const {
		return *m_connection.get();
	}

	const std::unordered_map<ServiceID, SomeIPProxy*> getProxies() const {
		return m_proxyTable;
	}

	const std::unordered_map<ServiceID, SomeIPStubAdapter*> getServices() const {
		return m_serviceTable;
	}

	bool registerService(const std::shared_ptr<StubBase>& stubBase, const char* interfaceId, const std::string& participantId,
			     const std::string& serviceName, const std::string& domain,
			     const std::shared_ptr<Factory>& factory) override;

	bool unregisterService(const std::string& serviceAddress) override {
		assert(false);
		return false;
	}

private:
	std::unique_ptr<SomeIPClient::ClientConnection> m_connection;
	std::unordered_map<ServiceID, SomeIPStubAdapter*> m_serviceTable;
	std::unordered_map<ServiceID, SomeIPProxy*> m_proxyTable;
	std::unordered_map<RequestID, MessageSink*> m_messageHandlers;

	SomeIPConnectionWatch m_watch;

	std::shared_ptr<MainLoopContext> m_mainLoopContext;

};

}
}
