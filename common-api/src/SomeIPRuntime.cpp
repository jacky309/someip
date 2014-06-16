#include "SomeIPRuntime.h"

#include "CommonAPI/Runtime.h"
#include "CommonAPI/MiddlewareInfo.h"

namespace CommonAPI {
namespace SomeIP {

CommonAPI::MiddlewareInfo middlewareInfo = CommonAPI::SomeIP::SomeIPRuntime::middlewareInfo_;

const CommonAPI::MiddlewareInfo SomeIPRuntime::middlewareInfo_( "SomeIP", &SomeIPRuntime::getInstanceAsGenericRuntime,
								Version(1, 0) );

struct SomeIPRegistration {
	SomeIPRegistration() {
		Runtime::registerRuntimeLoader(SomeIPRuntime::middlewareInfo_.middlewareName_,
					       &SomeIPRuntime::getInstanceAsGenericRuntime);
	}
};
static SomeIPRegistration registration__;

std::shared_ptr<SomeIPRuntime> SomeIPRuntime::getInstance() {
	static std::shared_ptr<SomeIPRuntime> singleton_( std::make_shared<SomeIPRuntime>() );
	if (!singleton_) {
		//		SomeIPRuntime s;
		//		singleton_ = std::make_shared<SomeIPRuntime>();
	}
	return singleton_;
}

std::shared_ptr<Runtime> SomeIPRuntime::getInstanceAsGenericRuntime() {
	return getInstance();
}

std::shared_ptr<Factory> SomeIPRuntime::createFactory(std::shared_ptr<MainLoopContext> mainLoopContext) {
	std::shared_ptr<Factory> factory = std::make_shared<SomeIPFactory> (getInstance(), mainLoopContext);
	return factory;
}

SomeIPReturnCode SomeIPFactory::initializeConnection() {
	SomeIPConnection& connection = m_runtime->getSomeIPConnection();
	if ( !connection.isConnected() ) {
		auto returnCode = connection.connect();

		if ( isError(returnCode) )
			return returnCode;

		MainLoopContext* mainLoopContext = m_mainLoopContext.get();
		if (mainLoopContext != nullptr)
			mainLoopContext->registerWatch( &connection.getWatch() );
		else {
			log_warning() <<
			"You are not using any main loop integration. Our recommendation is to use a main loop integration so that the dispatching of incoming messages is done by your main thread.";
			auto receiveThread = new std::thread( [&] () {

								      while( connection.getConnection().isConnected() ) {

									      std::array<struct pollfd, 1> fds;
									      int timeout_msecs = -1;

									      fds[0].fd =
										      connection.getConnection().
										      getFileDescriptor();
									      fds[0].events = POLLIN | POLLPRI | POLLHUP |
											      POLLERR;

									      auto ret =
										      poll(fds.data(), fds.size(), timeout_msecs);

									      if (ret > 0) {
										      if(fds[0].revents & POLLIN) {
											      connection.getConnection().
											      dispatchIncomingMessages();
										      } else if (fds[0].revents & POLLHUP) {
											      log_warn() <<
											      "Wake up disconnected";
											      connection.getConnection().
											      disconnect();
										      }
									      } else if (ret < 0)   {
										      log_error() <<
										      "Error during poll(). Terminating reception thread";
										      connection.getConnection().disconnect();
									      }
								      }

							      });
		}

		// TODO
		//		SomeIPClient::defaultContextGlibIntegrationSingleton([&]() {
		//									     auto& proxiesTable = connection.getProxies();
		//									     log_info("Pinging proxies");
		//									     for ( auto it = proxiesTable.begin();
		//										   it != proxiesTable.end(); ++it ) {
		//										     auto& proxy = it->second;
		//										     if ( proxy->isAvailable() ) {
		//											     log_info("Pinging proxy");
		//											     proxy->sendPingMessage( proxy->
		//														     getServiceID() );
		//										     }
		//									     }
		//								     }).setup();

	}

	return SomeIPReturnCode::OK;
}

std::shared_ptr<Proxy> SomeIPFactory::createProxy(const char* interfaceId, const std::string& participantId,
						  const std::string& serviceName, const std::string& domain) {

	for (auto it = registeredProxyFactoryFunctions.begin(); it != registeredProxyFactoryFunctions.end(); ++it) {
		if (it->first == interfaceId) {
			auto returnCode = initializeConnection();

			if ( isError(returnCode) )
				return nullptr;

			SomeIPConnection& connection = m_runtime->getSomeIPConnection();

			auto someIPProxy = (it->second)(connection, serviceName);
			someIPProxy->init();

			std::shared_ptr<Proxy> proxy = std::static_pointer_cast<Proxy> (someIPProxy);
			return proxy;
		}
	}

	return std::shared_ptr<Proxy>(nullptr);
}

std::shared_ptr<SomeIPStubAdapter> SomeIPFactory::createAdapter(const std::shared_ptr<StubBase>& service, const char* interfaceId,
								const std::string& participantId, const std::string& serviceName,
								const std::string& domain) {

	for (auto it = registeredAdapterFactoryFunctions.begin(); it != registeredAdapterFactoryFunctions.end(); ++it) {
		if (it->first == interfaceId) {
			auto returnCode = initializeConnection();

			if ( isError(returnCode) )
				return nullptr;

			SomeIPConnection& connection = m_runtime->getSomeIPConnection();

			auto someIPProxy = (it->second)(service, connection, serviceName);
			someIPProxy->init();

			return someIPProxy;
		}
	}

	return std::shared_ptr<SomeIPStubAdapter>(nullptr);
}

}
}
