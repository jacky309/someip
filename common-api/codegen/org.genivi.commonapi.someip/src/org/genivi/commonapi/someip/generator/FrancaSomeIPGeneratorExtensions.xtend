package org.genivi.commonapi.someip.generator

import javax.inject.Inject
import org.franca.core.franca.FAttribute
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions
import org.franca.core.franca.FStructType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FType
import org.franca.core.franca.FModelElement
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import org.franca.deploymodel.core.FDeployedInterface

class FrancaSomeIPGeneratorExtensions {
    @Inject private extension FTypeGeneratorMadePublic
//    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions
    
    def someipSetMethodName(FAttribute fAttribute) {
        'set' + fAttribute.className
    }

    def someipSignalName(FAttribute fAttribute) {
        'on' + fAttribute.className + 'Changed'
    }

    def someipGetMethodName(FAttribute fAttribute) {
        'get' + fAttribute.className
    }

    def changeNotificationMemberID(FAttribute fAttribute, FDInterface interfaceDeployment) {
		for (attributeDepl : interfaceDeployment.attributes)
        	if (attributeDepl.target == fAttribute)
    	    	if (deploymentProperties(interfaceDeployment).getValueChangeMemberID(fAttribute) != null)
	    	    	return deploymentProperties(interfaceDeployment).getValueChangeMemberID(fAttribute).toString();

    	var interface_ = fAttribute.containingInterface
		var index = interface_.attributes.indexOf(fAttribute)
		return '0x3000 + ' + (index * 3).toString
    }

    def getterMemberID(FAttribute fAttribute, FDInterface interfaceDeployment) {
		for (methodDepl : interfaceDeployment.attributes)
        	if (methodDepl.target == fAttribute)
	        	if (deploymentProperties(interfaceDeployment).getGetterMemberID(fAttribute) != null)
		        	return deploymentProperties(interfaceDeployment).getGetterMemberID(fAttribute).toString();
		
		return fAttribute.changeNotificationMemberID(interfaceDeployment) + '1'
    }

    def setterMemberID(FAttribute fAttribute, FDInterface interfaceDeployment) {
		for (methodDepl : interfaceDeployment.attributes)
        	if (methodDepl.target == fAttribute)
	        	if (deploymentProperties(interfaceDeployment).getSetterMemberID(fAttribute)!=null)
		        	return deploymentProperties(interfaceDeployment).getSetterMemberID(fAttribute).toString();

		return fAttribute.changeNotificationMemberID(interfaceDeployment) + '2'
    }
    
    def private deploymentProperties(FDInterface interfaceDeployment) {
    	var deployedInterface = new FDeployedInterface(interfaceDeployment)
		return new someip.ServiceSpecificationInterfacePropertyAccessor(deployedInterface)
     }

    def memberID(FMethod fMethod, FDInterface interfaceDeployment) {
        for (methodDepl : interfaceDeployment.methods)
        	if (methodDepl.target == fMethod)
	        	return deploymentProperties(interfaceDeployment).getMemberID(fMethod).toString();

		// No ID set in deployment file => assign one
      	var interface_ = fMethod.containingInterface
		var index = interface_.methods.indexOf(fMethod)
		return '0x1000 + ' + index.toString

    }

    def memberID(FBroadcast fBroadcast, FDInterface interfaceDeployment) {
        for (broadcastDepl : interfaceDeployment.broadcasts)
        	if (broadcastDepl.target == fBroadcast)
	        	return deploymentProperties(interfaceDeployment).getMemberID(fBroadcast).toString();

        	var interface_ = fBroadcast.containingInterface
			var index = interface_.broadcasts.indexOf(fBroadcast)
			return '0x2000 + ' + index.toString

    }

    def serviceID(FDInterface fInterface) {
		var id = deploymentProperties(fInterface).getServiceID(fInterface.target)
		return id
    }

    def commonAPIAddress(FDInterface fInterface) {
		return 'someip:' + fInterface.serviceID
    }

    def dispatch generateFTypeSerializer(FType fType, FModelElement parent) '''
// unknown type
//template<> inline void readCustomType(SomeIPInputStream& inputStream, «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {}
'''

    def dispatch generateFTypeSerializer(FEnumerationType fType, FModelElement parent) '''
// enum type
inline SomeIPInputStream& operator>>(SomeIPInputStream& stream, «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {
//template<> inline void readCustomType(SomeIPInputStream& inputStream, «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {
	stream.readEnum(v);
	return stream;
}

// enum type
inline SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {
	stream.writeEnum(v);
	return stream;
}
'''

    def dispatch generateFTypeSerializer(FStructType fType, FModelElement parent) '''
// struct type
inline SomeIPInputStream& operator>>(SomeIPInputStream& stream, «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {
//template<> inline void readCustomType(SomeIPInputStream& inputStream, «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {
	«FOR element : fType.elements»
	    stream >> v.«element.name»;
	«ENDFOR»
	return stream;
}

// enum type
inline SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {
//template<> inline void writeCustomType(SomeIPOutputStream& outputStream, const «fType.containingInterface.model.generateCppNamespace»«getClassNamespace(fType, parent)»& v) {
    «FOR element : fType.elements»
        stream << v.«element.name»;
    «ENDFOR»
	return stream;
}
'''

    def internal_compilation_guard() '''
        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif
    '''

}