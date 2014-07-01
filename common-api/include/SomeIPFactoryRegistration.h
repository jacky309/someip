#pragma once

#include "SomeIPRuntime.h"

namespace CommonAPI {
namespace SomeIP {

template<typename ProxyType, typename InterfaceType>
class ProxyFactoryRegistration {

	LOG_SET_CLASS_CONTEXT(someIPCommonAPILogContext);

public:
	ProxyFactoryRegistration() {
		SomeIPRuntime::getInstance()->registerProxyFactoryMethod(InterfaceType::getInterfaceId(), &createSomeIPProxy);
	}

	static std::shared_ptr<SomeIPProxy> createSomeIPProxy(SomeIPConnection& someipProxyConnection,
							      const std::string& commonApiAddress) {
		log_debug("Creating proxy");
		return std::make_shared<ProxyType>(someipProxyConnection, commonApiAddress);
	}

};

template<typename StubAdapterType, typename InterfaceType>
class StubAdapterFactoryRegistration {

	LOG_SET_CLASS_CONTEXT(someIPCommonAPILogContext);

public:
	StubAdapterFactoryRegistration() {
		SomeIPRuntime::getInstance()->registerAdapterFactoryMethod(InterfaceType::getInterfaceId(),
									   &createSomeIPStubAdapter);
	}

	static std::shared_ptr<SomeIPStubAdapter> createSomeIPStubAdapter(
		const std::shared_ptr<CommonAPI::StubBase>& stubBase,
		SomeIPConnection& connection,
		const std::string& commonApiAddress) {
		log_debug("Creating stub adapter");
		return std::make_shared<StubAdapterType>(stubBase, connection, commonApiAddress);
	}

};

}
}
