#pragma once

#include "CommonAPI/Runtime.h"
#include "CommonAPI/MiddlewareInfo.h"

#include "SomeIPProxy.h"
#include "SomeIP-clientLib.h"

namespace CommonAPI {
namespace SomeIP {

LOG_IMPORT_CONTEXT(someIPCommonAPILogContext);

using namespace CommonAPI;

class SomeIPClientID : public CommonAPI::ClientId {

public:
	~SomeIPClientID() override {
	}

	bool operator==(ClientId& clientIdToCompare) override {
		return true;
	}

	std::size_t hashCode() override {
		return 1;
	}

};

inline std::shared_ptr<ClientId> getSomeIPClientID(const InputMessage& msg) {
	return std::make_shared<SomeIPClientID>();
}

class SomeIPRuntime : public Runtime {

public:
	SomeIPRuntime() :
		m_connection( std::make_shared<SomeIPConnection> (m_clientLibConnection) ) {
	}

	static std::shared_ptr<SomeIPRuntime> getInstance();
	static std::shared_ptr<Runtime> getInstanceAsGenericRuntime();

	std::shared_ptr<Factory> createFactory( std::shared_ptr<MainLoopContext> = std::shared_ptr<MainLoopContext> (nullptr) );

	std::shared_ptr<Factory> doCreateFactory(std::shared_ptr<MainLoopContext> mainLoopContext,
						 const std::string& factoryName,
						 const bool nullOnInvalidName) override {
		return createFactory(mainLoopContext);
	}

	std::shared_ptr<CommonAPI::ServicePublisher> getServicePublisher() override {
		return m_connection;
		//		return std::shared_ptr<CommonAPI::ServicePublisher>(&getSomeIPConnectionSingleton());
		//		assert(false);
	}

	static const CommonAPI::MiddlewareInfo middlewareInfo_;

	SomeIPConnection& getSomeIPConnection() {
		return *( m_connection.get() );
	}

	SomeIPClient::ClientConnection m_clientLibConnection;
	std::shared_ptr<SomeIPConnection> m_connection;
};

class SomeIPFactory : public Factory {

	typedef std::shared_ptr<SomeIPStubAdapter> (*AdapterFactoryFunction)(const std::shared_ptr<StubBase>& service,
									     SomeIPConnection& connection,
									     const std::string& commonApiAddress
	                                                                     //								       , ServiceID serviceID
									     );

	typedef std::shared_ptr<SomeIPProxy> (*ProxyFactoryFunction)(SomeIPConnection& someipProxyConnection,
								     const std::string& commonApiAddress
	                                                             //			, CommonAPI::SomeIP::ServiceID serviceID
								     );

public:
	SomeIPFactory(std::shared_ptr<SomeIPRuntime> runtime, std::shared_ptr<MainLoopContext> mainLoopContext) :
		Factory(runtime, &SomeIPRuntime::middlewareInfo_) {
		m_runtime = runtime;
		m_mainLoopContext = mainLoopContext;
	}

	void initializeConnection();

	std::vector<std::string> getAvailableServiceInstances(const std::string& serviceName,
							      const std::string& serviceDomainName = "local") {
		// TODO : implement
		assert(false);
		std::vector<std::string> v;
		return v;
	}

	bool isServiceInstanceAlive(const std::string& serviceAddress) {
		// TODO : implement
		assert(false);
		return false;
	}

	bool isServiceInstanceAlive(const std::string& serviceInstanceID, const std::string& serviceName,
				    const std::string& serviceDomainName = "local") {
		// TODO : implement
		assert(false);
		return false;
	}

	std::shared_ptr<Proxy> createProxy(const char* interfaceId, const std::string& participantId,
					   const std::string& serviceName,
					   const std::string& domain);

	std::shared_ptr<SomeIPStubAdapter> createAdapter(const std::shared_ptr<StubBase>& service, const char* interfaceId,
							 const std::string& participantId, const std::string& serviceName,
							 const std::string& domain);

	bool registerAdapter(std::shared_ptr<StubBase> stubBase, const char* interfaceId, const std::string& participantId,
			     const std::string& serivceName, const std::string& domain) {
		// TODO : implement
		assert(false);
		return false;
	}

	bool unregisterService(const std::string& participantId, const std::string& serviceName, const std::string& domain) {
		// TODO : implement
		assert(false);
		return false;
	}


	/**
	 * \brief Get all instances of a specific service name available. Asynchronous call.
	 *
	 * Get all instances of a specific service name available. Asynchronous call.
	 *
	 * @param serviceName The service name of the common API address (middle part)
	 * @param serviceDomainName The domain of the common API address (first part)
	 */
	void getAvailableServiceInstancesAsync(GetAvailableServiceInstancesCallback callback, const std::string& serviceName,
					       const std::string& serviceDomainName = "local") override {
		assert(false);
	}


	/**
	 * \brief Tells whether a particular service instance is available. Asynchronous call.
	 *
	 * Tells whether a particular service instance is available. Asynchronous call.
	 *
	 * @param serviceAddress The common API address of the service
	 */
	void isServiceInstanceAliveAsync(IsServiceInstanceAliveCallback callback, const std::string& serviceAddress) override {
		assert(false);
	}


	/**
	 * \brief Tells whether a particular service instance is available. Asynchronous call.
	 *
	 * Tells whether a particular service instance is available. Asynchronous call.
	 *
	 * @param serviceInstanceID The participant ID of the common API address (last part) of the service
	 * @param serviceName The service name of the common API address (middle part) of the service
	 * @param serviceDomainName The domain of the common API address (first part) of the service
	 */
	void isServiceInstanceAliveAsync(IsServiceInstanceAliveCallback callback, const std::string& serviceInstanceID,
					 const std::string& serviceName,
					 const std::string& serviceDomainName = "local") override {
		assert(false);
	}

	static void registerAdapterFactoryMethod(const std::string& interfaceID, AdapterFactoryFunction function);

	static void registerProxyFactoryMethod(const std::string& interfaceID, ProxyFactoryFunction function);

private:
	static std::unordered_map<std::string, ProxyFactoryFunction> registeredProxyFactoryFunctions;
	static std::unordered_map<std::string, AdapterFactoryFunction> registeredAdapterFactoryFunctions;

	std::shared_ptr<SomeIPRuntime> m_runtime;
	std::shared_ptr<MainLoopContext> m_mainLoopContext;
};

}
}