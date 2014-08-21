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

	class CommonAPIIdleMainLoopHook : public IdleMainLoopHook {

public:
		CommonAPIIdleMainLoopHook(CallBackFunction callBackFunction, std::shared_ptr<MainLoopContext> mainLoopContext) {
			m_callBack = callBackFunction;
			m_mainLoopContext = mainLoopContext;
		}

		void activate() override {
			assert(false);
		}

private:
		CallBackFunction m_callBack;
		std::shared_ptr<MainLoopContext> m_mainLoopContext;
	};

	class CommonAPITimeOutMainLoopHook : public TimeOutMainLoopHook, private CommonAPI::Timeout {
public:
		CommonAPITimeOutMainLoopHook(CallBackFunction callBackFunction, int durationInMilliseconds,
					     std::shared_ptr<MainLoopContext> mainLoopContext) {
			m_mainLoopContext = mainLoopContext;
			m_callBack = callBackFunction;
			m_durationInMilliseconds = durationInMilliseconds;
		}

		~CommonAPITimeOutMainLoopHook() override {
			m_mainLoopContext->deregisterTimeoutSource(this);
		}

		void activate() override {
			m_mainLoopContext->registerTimeoutSource(this);
		}

	    bool dispatch() override {
	    	m_callBack();
	    	return false;
	    }

	    int64_t getTimeoutInterval() const override {
	    	return m_durationInMilliseconds;
	    }

	    int64_t getReadyTime() const override {
	    	return m_durationInMilliseconds;
	    }

private:
		CallBackFunction m_callBack;
		std::shared_ptr<MainLoopContext> m_mainLoopContext;
		int m_durationInMilliseconds;
	};

	class CommonAPIWatchMainLoopHook : public WatchMainLoopHook, private Watch {
public:
		CommonAPIWatchMainLoopHook(CallBackFunction callBackFunction, const pollfd& fd,
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
		const pollfd& m_fd;
		std::vector<DispatchSource*> m_dependentSources;
	};

public:
	SomeIPConnection(SomeIPClient::ClientConnection* clientConnection) : m_connection(clientConnection) {
		//,		m_watch( getConnection() )
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

	std::unique_ptr<WatchMainLoopHook> addWatch(WatchMainLoopHook::CallBackFunction callBackFunction,
						    const pollfd& fd) override {
		return std::unique_ptr<WatchMainLoopHook>( new CommonAPIWatchMainLoopHook(callBackFunction, fd, m_mainLoopContext) );
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

	std::shared_ptr<MainLoopContext> m_mainLoopContext;

};

}
}
