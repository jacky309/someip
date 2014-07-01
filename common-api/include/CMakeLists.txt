
set(INCLUDE_FILES
CommonAPI-SomeIP.h
SerializableArguments.h
SomeIPHelper.h
StubAdapterHelper.h
SomeIPConnection.h
SomeIPProxy.h
SomeIPEvent.h
SomeIPRuntime.h
ProxyHelper.h
Attribute.h
SomeIPFactoryRegistration.h
SomeIPStubAdapter.h
GlibMainLoopContextHandler.h
)

install(FILES ${INCLUDE_FILES} DESTINATION include/someip/CommonApi)
