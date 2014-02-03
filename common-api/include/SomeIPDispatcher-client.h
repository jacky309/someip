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

class SomeIPConnection : private SomeIPClient::ClientConnectionListener, public CommonAPI::ServicePublisher {

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
			log_info() << eventFlags;
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

			//	        if(channelFlags_ & DBUS_WATCH_READABLE) {
			//	            pollFlags |= POLLIN;
			//	        }
			//	        if(channelFlags_ & DBUS_WATCH_WRITABLE) {
			//	            pollFlags |= POLLOUT;
			//	        }
			//	        pollFileDescriptor_.fd = dbus_watch_get_unix_fd(libdbusWatch_);
			//	        pollFileDescriptor_.events = pollFlags;
			//	        pollFileDescriptor_.revents = 0;

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

		pollfd m_pollfd;
		std::vector<DispatchSource*> m_dependentDispatchSources;

		SomeIPClient::ClientConnection& m_connection;

		SomeIPDispatchSource m_dispatchSource;

		friend class SomeIPConnection;
	};

public:
	SomeIPConnection(SomeIPClient::ClientConnection& clientConnection) : m_connection(clientConnection),
		m_watch(clientConnection) {
	}

	Watch& getWatch() {
		return m_watch;
	}

	void connect() {
		m_connection.connect(*this);
	}

	bool isConnected() {
		return m_connection.isConnected();
	}

	void onDisconnected() {
		log_info();
	}

	SomeIPFunctionReturnCode sendMessage(OutputMessage& msg) {
		return m_connection.sendMessage(msg);
	}

	InputMessage sendMessageBlocking(OutputMessage& msg) {
		return m_connection.sendMessageBlocking(msg);
	}

	//	bool sendWithResponse(SomeIPClient::OutputMessageWithReport& msg) {
	//		return m_connection.sendWithResponse(msg);
	//	}

	void sendWithResponseHandler(OutputMessage& msg,
				     MessageSink& messageHandler) {
		m_messageHandlers[msg.getHeader().getRequestID()] = &messageHandler;
		sendMessage(msg);
	}

	void registerService(SomeIPStubAdapter& service);

	void registerProxy(SomeIPProxy& proxy);

	void unregisterService(ServiceID serviceID) {
		getConnection().unregisterService(serviceID);
	}

	//	void unregisterProxy(ServiceID serviceID) {
	//		getConnection().unregisterProxy(serviceID);
	//	}

	void subscribeNotification(MessageID messageID) {
		m_connection.subscribeToNotifications(messageID);
	}

	MessageProcessingResult processMessage(const InputMessage& message);

	SomeIPClient::ClientConnection& getConnection() {
		return m_connection;
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
	SomeIPClient::ClientConnection& m_connection;
	std::unordered_map<ServiceID, SomeIPStubAdapter*> m_serviceTable;
	std::unordered_map<ServiceID, SomeIPProxy*> m_proxyTable;
	std::unordered_map<RequestID, MessageSink*> m_messageHandlers;

	SomeIPConnectionWatch m_watch;

};

}
}
