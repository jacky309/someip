#include "SomeIPRuntime.h"

#include "CommonAPI/Runtime.h"
#include "CommonAPI/MiddlewareInfo.h"

#include "GlibMainLoopContextHandler.h"

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
	static SomeIPRegistration s_instance;
};

SomeIPRegistration SomeIPRegistration::s_instance;

std::shared_ptr<SomeIPRuntime> SomeIPRuntime::getInstance() {
	static auto singleton_( std::make_shared<SomeIPRuntime>() );
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

		if (m_mainLoopContext.get() == nullptr) {
			log_warning() <<
			"You are not using any main loop integration. Our recommendation is to use a main loop integration so that the dispatching of incoming messages is done by your main thread.";
			m_mainLoopContext = getRuntime()->getNewMainLoopContext();
			auto mainLoop = new GlibMainLoopContextHandler(m_mainLoopContext);
			std::thread* m_receiveThread = new std::thread([&] () {
									       mainLoop->runMainLoop();
								       });

		}

		auto returnCode = connection.connect(m_mainLoopContext);

		if ( isError(returnCode) )
			return returnCode;

		//		m_mainLoopContext->registerWatch( &connection.getWatch() );

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

	for (auto it = m_runtime->getProxyFactories().begin(); it != m_runtime->getProxyFactories().end(); ++it) {
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

	for (auto it = m_runtime->getAdapterFactories().begin(); it != m_runtime->getAdapterFactories().end(); ++it) {
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
