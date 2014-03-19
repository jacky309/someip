package org.genivi.commonapi.someip.generator

import javax.inject.Inject
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModelElement
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor

import static com.google.common.base.Preconditions.*
import org.franca.deploymodel.dsl.fDeploy.FDInterface

class FInterfaceSomeIPProxyGenerator {
    @Inject private extension FrancaGeneratorExtensions
    @Inject private extension FrancaSomeIPGeneratorExtensions

	def generateSomeIPProxy(FDInterface fDInterface, IFileSystemAccess fileSystemAccess) {
        var fInterface = fDInterface.target
        fileSystemAccess.generateFile(fInterface.someipProxyHeaderPath, fDInterface.generateSomeIPProxyHeader(null))
        fileSystemAccess.generateFile(fInterface.someipProxySourcePath, fDInterface.generateSomeIPProxySource(null))
	}

    def private generateSomeIPProxyHeader(FDInterface fDInterface, DeploymentInterfacePropertyAccessor deploymentAccessor) '''

        «var fInterface = fDInterface.target»

        #pragma once

		«internal_compilation_guard»

		#include "SomeIPProxy.h"
		#include "ProxyHelper.h"

		#include "«fInterface.name»SomeIPStubAdapter.h"   // For custom type serializer definitions. TODO : move to a common files

        #include <«fInterface.proxyBaseHeaderPath»>

        «IF fInterface.hasAttributes»
        «ENDIF»
        «IF fInterface.hasBroadcasts»
        «ENDIF»

        #include <string>

        «fInterface.model.generateNamespaceBeginDeclaration»
        
//        const char* «fInterface.name»::getInterfaceId() {
//            return "«fInterface.fullyQualifiedName»";
//        }

        class «fInterface.someipProxyClassName»: virtual public «fInterface.proxyBaseClassName», virtual public CommonAPI::SomeIP::SomeIPProxy {
         public:
            «fInterface.someipProxyClassName»(CommonAPI::SomeIP::SomeIPConnection& someipProxyconnection,
                            const std::string& commonApiAddress = "«fDInterface.commonAPIAddress»",
                            CommonAPI::SomeIP::ServiceID serviceID = «fDInterface.serviceID»);

            virtual ~«fInterface.someipProxyClassName»() { }

            «FOR attribute : fInterface.attributes»
                virtual «attribute.generateGetMethodDefinition»;
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                virtual «broadcast.generateGetMethodDefinition»;
            «ENDFOR»

            «FOR method : fInterface.methods»

                virtual «method.generateDefinition»;
                «IF !method.isFireAndForget»
                    virtual «method.generateAsyncDefinition»;
                «ENDIF»
            «ENDFOR»
            
            virtual void getOwnVersion(uint16_t& ownVersionMajor, uint16_t& ownVersionMinor) const;

         private:
            «FOR attribute : fInterface.attributes»
                «attribute.someipClassName» «attribute.someipClassVariableName»;
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                «broadcast.someipClassName» «broadcast.someipClassVariableName»;
            «ENDFOR»
        };

        «fInterface.model.generateNamespaceEndDeclaration»

    '''

    def private generateSomeIPProxySource(FDInterface fDInterface, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «var fInterface = fDInterface.target»

		«internal_compilation_guard»

        #include "«fInterface.someipProxyHeaderFile»"
		#include "SomeIPFactoryRegistration.h"

        «fInterface.model.generateNamespaceBeginDeclaration»

        static CommonAPI::SomeIP::ProxyFactoryRegistration<«fInterface.someipProxyClassName», «fInterface.name»> s_factory;

        «fInterface.someipProxyClassName»::«fInterface.someipProxyClassName»(
                            CommonAPI::SomeIP::SomeIPConnection& someipProxyconnection,
                            const std::string& commonApiAddress,
                            CommonAPI::SomeIP::ServiceID serviceID):
                CommonAPI::SomeIP::SomeIPProxy(someipProxyconnection, commonApiAddress, serviceID)
                «FOR attribute : fInterface.attributes BEFORE ',' SEPARATOR ','»
                    «attribute.generateSomeIPVariableInit(fDInterface)»
                «ENDFOR»
                «FOR broadcast : fInterface.broadcasts BEFORE ',' SEPARATOR ','»
                    «broadcast.someipClassVariableName»(*this, «broadcast.memberID(fDInterface)»)
                «ENDFOR» {
        }

        «FOR attribute : fInterface.attributes»
            «attribute.generateGetMethodDefinitionWithin(fInterface.someipProxyClassName)» {
                return «attribute.someipClassVariableName»;
            }
        «ENDFOR»

        «FOR broadcast : fInterface.broadcasts»
            «broadcast.generateGetMethodDefinitionWithin(fInterface.someipProxyClassName)» {
                return «broadcast.someipClassVariableName»;
            }
        «ENDFOR»

        «FOR method : fInterface.methods»
«««        	«var method = methodDeployment.target»
            «method.generateDefinitionWithin(fInterface.someipProxyClassName)» {
                «method.generateSomeIPProxyHelperClass»::callMethodWithReply(
                    *this,
                    «method.memberID(fDInterface)»,
                    «method.inArgs.map[name].join('', ', ', ', ', [toString])»
                    callStatus«IF method.hasError»,
                    methodError«ENDIF»
                    «method.outArgs.map[name].join(', ', ', ', '', [toString])»);
            }
            «IF !method.isFireAndForget»
                «method.generateAsyncDefinitionWithin(fInterface.someipProxyClassName)» {
                    return «method.generateSomeIPProxyHelperClass»::callMethodAsync(
                        *this,
                        «method.memberID(fDInterface)»,
                        «method.inArgs.map[name].join('', ', ', ', ', [toString])»
                        std::move(callback));
                }
            «ENDIF»
        «ENDFOR»
        
        void «fInterface.someipProxyClassName»::getOwnVersion(uint16_t& ownVersionMajor, uint16_t& ownVersionMinor) const {
            ownVersionMajor = «fInterface.version.major»;
            ownVersionMinor = «fInterface.version.minor»;
        }

        «fInterface.model.generateNamespaceEndDeclaration»
    '''

    def private someipClassVariableName(FModelElement fModelElement) {
        checkArgument(!fModelElement.name.nullOrEmpty, 'FModelElement has no name: ' + fModelElement)
        fModelElement.name.toFirstLower + '_'
    }

    def private someipProxyHeaderFile(FInterface fInterface) {
        fInterface.name + "SomeIPProxy.h"
    }

    def private someipProxyHeaderPath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.someipProxyHeaderFile
    }

    def private someipProxySourceFile(FInterface fInterface) {
        fInterface.name + "SomeIPProxy.cpp"
    }

    def private someipProxySourcePath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.someipProxySourceFile
    }

    def private someipProxyClassName(FInterface fInterface) {
        fInterface.name + 'SomeIPProxy'
    }

    def private generateSomeIPProxyHelperClass(FMethod fMethod) '''
        CommonAPI::SomeIP::SomeIPProxyHelper<CommonAPI::SomeIP::SerializableArguments<«fMethod.inArgs.map[type.getNameReference(fMethod.model)].join(', ')»>,
                                         CommonAPI::SomeIP::SerializableArguments<«IF fMethod.hasError»«fMethod.getErrorNameReference(fMethod.eContainer)»«IF !fMethod.outArgs.empty», «ENDIF»«ENDIF»«fMethod.outArgs.map[type.getNameReference(fMethod.model)].join(', ')»> >'''

    def private someipClassName(FAttribute fAttribute) {
        var type = 'CommonAPI::SomeIP::SomeIP'

        if (fAttribute.isReadonly)
            type = type + 'Readonly'

        type = type + 'Attribute<' + fAttribute.className + '>'

        if (fAttribute.isObservable)
            type = 'CommonAPI::SomeIP::SomeIPObservableAttribute<' + type + '>'

        return type
    }

    def private generateSomeIPVariableInit(FAttribute fAttribute, FDInterface fdInterface) {
        var ret = fAttribute.someipClassVariableName + '(*this'

        if (fAttribute.isObservable)
            ret = ret + ', ' + fAttribute.changeNotificationMemberID(fdInterface) + ''

        if (!fAttribute.isReadonly)
            ret = ret + ', ' + fAttribute.setterMemberID (fdInterface)

        ret = ret + ', ' + fAttribute.getterMemberID(fdInterface) + ')'

        return ret
    }

    def private someipClassName(FBroadcast fBroadcast) {
        return 'CommonAPI::SomeIP::SomeIPEvent<' + fBroadcast.className + '>'
    }
}
