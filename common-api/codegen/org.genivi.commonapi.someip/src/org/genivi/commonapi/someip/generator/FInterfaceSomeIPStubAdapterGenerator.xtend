package org.genivi.commonapi.someip.generator

import javax.inject.Inject
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModelElement
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import org.franca.core.franca.FModel
import org.franca.core.franca.FTypeCollection
import org.genivi.commonapi.core.generator.FrancaGenerator

class FInterfaceSomeIPStubAdapterGenerator {
    @Inject private extension FrancaGenerator
    @Inject private extension FrancaGeneratorExtensions
    @Inject private extension FrancaSomeIPGeneratorExtensions
	@Inject private extension FTypeGeneratorMadePublic = new FTypeGeneratorMadePublic

	def generateSomeIPStubAdapter(FDInterface fDInterface, FModel model, IFileSystemAccess fileSystemAccess) {
        var fInterface = fDInterface.target
        fileSystemAccess.generateFile(fInterface.someipStubAdapterHeaderPath, fDInterface.generateSomeIPStubAdapterHeader(model, null))
        fileSystemAccess.generateFile(fInterface.someipStubAdapterSourcePath, fDInterface.generateSomeIPStubAdapterSource(model, null))
	}

	def getReferencedType(FModel model) {
		    val allReferencedFTypes = model.allReferencedFTypes
	        val allFTypeTypeCollections = allReferencedFTypes.filter[eContainer instanceof FTypeCollection].map[
	            eContainer as FTypeCollection]
	
	        val generateTypeCollections = model.typeCollections.toSet
	        generateTypeCollections.addAll(allFTypeTypeCollections)
		
		return generateTypeCollections;
	}

	def getSomeIPDefinitionFilePath(FModel model, FTypeCollection collection) {
		return model.directoryPath + '/' + collection.name + 'SomeIP.h';
	}

	def generateTypeCollectionSomeIP(FModel model, IFileSystemAccess fileSystemAccess) {

        val generateTypeCollections = getReferencedType(model)

        generateTypeCollections.forEach [
        	var collection = it
	        fileSystemAccess.generateFile(getSomeIPDefinitionFilePath(model, collection), collection.generateTypeCollectionSomeIP())
        ]

	}

	def generateTypeCollectionSomeIP(FTypeCollection collection) '''
		
		#include "«collection.name».h"
		
		namespace SomeIP_Lib {

		«FOR type: collection.types.sortTypes(null)»
            «type.generateFTypeSerializer(collection)»
        «ENDFOR»
		
		}
		
	'''

    def private generateSomeIPStubAdapterHeader(FDInterface fDInterface, FModel model, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «val fInterface = fDInterface.target»
        #pragma once

		«internal_compilation_guard»

        #include "StubAdapterHelper.h"

        #include <«fInterface.stubHeaderPath»>

////
        «FOR collection : getReferencedType(model)»
	        #include "«getSomeIPDefinitionFilePath(model, collection)»"
		«ENDFOR»
////

        «fInterface.model.generateNamespaceBeginDeclaration»

        typedef CommonAPI::SomeIP::StubAdapterHelper<«fInterface.stubClassName»> «fInterface.someipStubAdapterHelperClassName»;

        class «fInterface.someipStubAdapterClassName»: public «fInterface.stubAdapterClassName», public «fInterface.someipStubAdapterHelperClassName» {
         public:
            «fInterface.someipStubAdapterClassName»(
                    const std::shared_ptr<CommonAPI::StubBase>& stub,
                    CommonAPI::SomeIP::SomeIPConnection& connection,
                    const std::string& commonApiAddress = "«fDInterface.commonAPIAddress»",
                    CommonAPI::SomeIP::ServiceID serviceID = «fDInterface.serviceID»);
            
            «FOR attribute : fInterface.attributes»
                «IF attribute.isObservable»
                    void «attribute.stubAdapterClassFireChangedMethodName»(const «attribute.type.getNameReference(fInterface.model)»& value);
                «ENDIF»
            «ENDFOR»
        
            «FOR broadcast: fInterface.broadcasts»
                void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + type.getNameReference(fInterface.model) + '& ' + name].join(', ')»);
            «ENDFOR»
    
		    void deactivateManagedInstances() override {
		    }
    
        };

        «fInterface.model.generateNamespaceEndDeclaration»

    '''

    def private generateSomeIPStubAdapterSource(FDInterface fDInterface, FModel model, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «val fInterface = fDInterface.target»
        
        «internal_compilation_guard»
        
        #include "«fInterface.someipStubAdapterHeaderFile»"
        #include <«fInterface.headerPath»>
		#include "SomeIPFactoryRegistration.h"

        «fInterface.model.generateNamespaceBeginDeclaration»
        
        static CommonAPI::SomeIP::StubAdapterFactoryRegistration<«fInterface.someipStubAdapterClassName», «fInterface.name»> s_factory;

        «fInterface.someipStubAdapterClassName»::«fInterface.someipStubAdapterClassName»(
                const std::shared_ptr<CommonAPI::StubBase>& stub,
                CommonAPI::SomeIP::SomeIPConnection& connection,
                const std::string& commonApiAddress,
                CommonAPI::SomeIP::ServiceID serviceID):
                «fInterface.someipStubAdapterHelperClassName»(std::dynamic_pointer_cast<«fInterface.stubClassName»>(stub), connection, commonApiAddress, serviceID) {
        }




        «FOR attribute : fInterface.attributes»
            static CommonAPI::SomeIP::GetAttributeStubDispatcher<
                    «fInterface.stubClassName»,
                    «attribute.type.getNameReference(fInterface.model)»
                    > «attribute.someipGetStubDispatcherVariable»(&«fInterface.stubClassName»::«attribute.stubClassGetMethodName»);
            «IF !attribute.isReadonly»
                static CommonAPI::SomeIP::Set«IF attribute.observable»Observable«ENDIF»AttributeStubDispatcher<
                        «fInterface.stubClassName»,
                        «attribute.type.getNameReference(fInterface.model)»
                        > «attribute.someipSetStubDispatcherVariable»(
                                &«fInterface.stubClassName»::«attribute.stubClassGetMethodName»,
                                &«fInterface.stubRemoteEventClassName»::«attribute.stubRemoteEventClassSetMethodName»,
                                &«fInterface.stubRemoteEventClassName»::«attribute.stubRemoteEventClassChangedMethodName»,
                                «IF attribute.observable»&«fInterface.stubAdapterClassName»::«attribute.stubAdapterClassFireChangedMethodName»«ENDIF»
                                );
            «ENDIF»
            
        «ENDFOR»
        
        «FOR method : fInterface.methods»
«««        	«var method = methodDeployment.target»
            «IF !method.isFireAndForget»
                static CommonAPI::SomeIP::MethodWithReplyStubDispatcher<
                    «fInterface.stubClassName»,
                    std::tuple<«method.allInTypes»>,
                    std::tuple<«method.allOutTypes»>
                    > «method.someipStubDispatcherVariable»(&«fInterface.stubClassName + "::" + method.name»);
            «ELSE»
                static CommonAPI::SomeIP::MethodStubDispatcher<
                    «fInterface.stubClassName»,
                    std::tuple<«method.allInTypes»>
                    > «method.someipStubDispatcherVariable»(&«fInterface.stubClassName + "::" + method.name»);
            «ENDIF»
            
        «ENDFOR»

        «FOR attribute : fInterface.attributes»
            «IF attribute.isObservable»
                void «fInterface.someipStubAdapterClassName»::«attribute.stubAdapterClassFireChangedMethodName»(const «attribute.type.getNameReference(fInterface.model)»& value) {
                	CommonAPI::SomeIP::StubSignalHelper<CommonAPI::SomeIP::SerializableArguments<«attribute.type.getNameReference(fInterface.model)»>>
                        ::sendSignal(
                            *this,
                            «attribute.changeNotificationMemberID(fDInterface)»,
                            value
                    );
                }
            «ENDIF»
        «ENDFOR»

        «FOR broadcast: fInterface.broadcasts»
«««        	«var broadcast = broadcastDeployment.target»
            void «fInterface.someipStubAdapterClassName»::«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + type.getNameReference(fInterface.model) + '& ' + name].join(', ')») {
                CommonAPI::SomeIP::StubSignalHelper<CommonAPI::SomeIP::SerializableArguments<«broadcast.outArgs.map[type.getNameReference(fInterface.model)].join(', ')»>>
                        ::sendSignal(
                            *this,
                            «broadcast.memberID(fDInterface)»
                            «IF broadcast.outArgs.size > 0»,«ENDIF»
                            «broadcast.outArgs.map[name].join(', ')»
                    );
            }
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»

        template<>
        const «fInterface.absoluteNamespace»::«fInterface.someipStubAdapterHelperClassName»::StubDispatcherTable «fInterface.absoluteNamespace»::«fInterface.someipStubAdapterHelperClassName»::stubDispatcherTable_ = {
            «FOR attribute : fInterface.attributes SEPARATOR ','»
                { «attribute.getterMemberID(fDInterface)», &«fInterface.absoluteNamespace»::«attribute.someipGetStubDispatcherVariable» }
                «IF !attribute.isReadonly»
                    , { «attribute.setterMemberID(fDInterface)» + 1, &«fInterface.absoluteNamespace»::«attribute.someipSetStubDispatcherVariable» }
                «ENDIF»
            «ENDFOR»
            «IF !fInterface.attributes.empty && !fInterface.methods.empty»,«ENDIF»
            «FOR method : fInterface.methods SEPARATOR ','»
                { «method.memberID(fDInterface)», &«fInterface.absoluteNamespace»::«method.someipStubDispatcherVariable» }
            «ENDFOR»
        };
        
    '''

    def private getAbsoluteNamespace(FModelElement fModelElement) {
        fModelElement.model.name.replace('.', '::')
    }

    def private someipStubAdapterHeaderFile(FInterface fInterface) {
        fInterface.name + "SomeIPStubAdapter.h"
    }

    def private someipStubAdapterHeaderPath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.someipStubAdapterHeaderFile
    }

    def private someipStubAdapterSourceFile(FInterface fInterface) {
        fInterface.name + "SomeIPStubAdapter.cpp"
    }

    def private someipStubAdapterSourcePath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.someipStubAdapterSourceFile
    }

    def private someipStubAdapterClassName(FInterface fInterface) {
        fInterface.name + 'SomeIPStubAdapter'
    }

    def private someipStubAdapterHelperClassName(FInterface fInterface) {
        fInterface.name + 'SomeIPStubAdapterHelper'
    }

    def private getAllInTypes(FMethod fMethod) {
        fMethod.inArgs.map[type.getNameReference(fMethod.model)].join(', ')
    }

    def private getAllOutTypes(FMethod fMethod) {
        var types = fMethod.outArgs.map[getTypeName(fMethod.model)].join(', ')

        if (fMethod.hasError) {
            if (!fMethod.outArgs.empty)
                types = ', ' + types
            types = fMethod.getErrorNameReference(fMethod.eContainer) + types
        }

        return types
    }

    def private someipStubDispatcherVariable(FMethod fMethod) {
        fMethod.name.toFirstLower + 'StubDispatcher'
    }

    def private someipGetStubDispatcherVariable(FAttribute fAttribute) {
        fAttribute.someipGetMethodName + 'StubDispatcher'
    }

    def private someipSetStubDispatcherVariable(FAttribute fAttribute) {
        fAttribute.someipSetMethodName + 'StubDispatcher'
    }
    
}
