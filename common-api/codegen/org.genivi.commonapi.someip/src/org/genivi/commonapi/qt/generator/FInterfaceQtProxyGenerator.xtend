package org.genivi.commonapi.qt.generator

import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FInterface
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor

import org.franca.core.franca.FTypeRef
import org.eclipse.emf.ecore.EObject
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FType
import org.franca.core.franca.FStructType
import org.franca.core.franca.FUnionType
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FMapType
import javax.inject.Inject
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FField
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions
import org.genivi.commonapi.core.generator.FTypeGenerator
import org.genivi.commonapi.someip.generator.ListBuilder
import org.franca.core.franca.FTypeDef
import java.util.ArrayList
import org.genivi.commonapi.someip.generator.FTypeGeneratorMadePublic

class QMLGeneratorExtentions extends FrancaGeneratorExtensions {

	def fullyQualifiedCppName(FInterface fInterface) {
		return fInterface.model.generateCppNamespace + fInterface.name
	}

	def qmlIdentifier(FInterface fInterface) {
		return fInterface.name.qmlIdentifier
	}

	def qmlIdentifier(FTypedElement attribute, FTypedElement parent) {
		if (parent != null)
			return parent.qmlIdentifier(null) + "_" + attribute.name.qmlIdentifier
		else
			return attribute.name.qmlIdentifier
	}

	def qmlIdentifier(FAttribute attribute) {
		return attribute.name.qmlIdentifier
	}

	def qmlIdentifier(FField attribute, FTypedElement parent) {
		if (parent != null)
			return parent.qmlIdentifier(null) + "_" + attribute.name.qmlIdentifier
		else
			return attribute.name.qmlIdentifier;
	}

	def qmlIdentifier(String name) {
		return name.toFirstLower
	}

	def qmlElementName(FTypedElement attribute, FTypedElement parent) {
		return attribute.qmlIdentifier(parent) + "Field"
	}

}

class QtGeneratorExtensions extends QMLGeneratorExtentions {

	def getQtProxyClassName(FInterface fInterface) {
		fInterface.name + 'QtProxy'
	}

	def typeDefinitionClassName(FInterface fInterface) {
		return fInterface.name + 'TypesDefinitions'
	}

	def getNameReference(FType type, EObject source) {
		return type.getRelativeNameReference(source)
	}

	def getQtWrapper(FTypeRef typeRef) {
		if (typeRef.derived == null)
			return new CommonAPIOtherQtWrapper(typeRef)
		else
			return getQtWrapper(typeRef.derived)
	}

	def CommonAPITypeQtWrapper getQtWrapper(FType type) {
		if (type instanceof FEnumerationType)
			return new CommonAPIEnumQtWrapper(type as FEnumerationType)
		if (type instanceof FStructType)
			return new CommonAPIStructQtWrapper(type as FStructType)
		if (type instanceof FUnionType)
			return new CommonAPIUnionQtWrapper(type as FUnionType)
		if (type instanceof FArrayType)
			return new CommonAPIArrayQtWrapper(type as FArrayType)
		if (type instanceof FMapType)
			return new CommonAPIMapQtWrapper(type as FMapType)
		if (type instanceof FMapType)
			return new CommonAPIMapQtWrapper(type as FMapType)
		if (type instanceof FTypeDef) {
			var typeDefType = type as FTypeDef
			return getQtWrapper(typeDefType.actualType)
		} else
			return new CommonAPIOtherQtWrapper(type)
	}

	def CommonAPITypeQtWrapper getQtWrapper(FEnumerationType type) {
		return new CommonAPIEnumQtWrapper(type)
	}

}

abstract class CommonAPITypeQtWrapper extends FrancaGeneratorExtensions {
	protected extension QtGeneratorExtensions = new QtGeneratorExtensions

	def fullyQualifiedTypeDefinitionClassName(FInterface fInterface) {
		return fInterface.model.generateCppNamespace + typeDefinitionClassName(fInterface)
	}

	def getQtWrapperName_() {
		return "unknown"
	}

	def abstract boolean needsWrapper() ;

	def generateTypeClassContribution() ''''''

	def abstract String getQtNameReference(EObject source) ;

	def getQtPrimitiveTypeName(FBasicTypeId fBasicTypeId) {
		switch fBasicTypeId {
			case FBasicTypeId::BOOLEAN: "bool"
			case FBasicTypeId::INT8: "int"
			case FBasicTypeId::UINT8: "unsigned int"
			case FBasicTypeId::INT16: "int"
			case FBasicTypeId::UINT16: "unsigned int"
			case FBasicTypeId::INT32: "int"
			case FBasicTypeId::UINT32: "unsigned int"
			case FBasicTypeId::INT64: "int"
			case FBasicTypeId::UINT64: "unsigned int"
			case FBasicTypeId::FLOAT: "float"
			case FBasicTypeId::DOUBLE: "double"
			case FBasicTypeId::STRING: "QString"
			case FBasicTypeId::BYTE_BUFFER: "CommonAPI::ByteBuffer"
			default: throw new IllegalArgumentException("Unsupported basic type: " + fBasicTypeId.getName)
		}
	}

	def getQtWrapperName(FType type, EObject source) {
		return type.name + "QtWrapper"
	}

	def abstract CharSequence fullyQualifiedQtWrapperName();

	def abstract String fullyQualifiedCppName();

	def generateDefaultNamespaceContribution() ''''''

	def generateQtWrapperClassDefinition(FInterface fInterface) ''''''

	def generateQtPropertyDefinition(FTypeRef type, String name, FInterface fInterface, boolean byReference,
		boolean isConst, boolean isNotifyable) '''
		 		Q_PROPERTY(«type.qtWrapper.fullyQualifiedQtWrapperName» «name» READ get«name» «if (!isConst) ''' WRITE set«name»'''» «if (isNotifyable) ''' NOTIFY valueChanged'''»);
		«if(isConst) 'Const'»CommonAPIQtDataWrapper«if(byReference) 'ByReference'» <«type.getNameReference(fInterface.model)»> m_«name»Wrapper;
		«type.qtWrapper.fullyQualifiedQtWrapperName» get«name»() {
			return CommonAPI2QtDataConverter<«type.qtWrapper.fullyQualifiedCppName»>::convertCommonAPI2Qt(m_«name»Wrapper); 
		}
		
		«if (!isConst) '''
void set«name»(const «type.qtWrapper.fullyQualifiedQtWrapperName»& value) {
	assignQtTypeToCommonAPI(value,  m_«name»Wrapper.getCommonAPIType());
}
 '''»
		
		const «type.getNameReference(fInterface.model)»& getCommonAPI«name»() const {
			return m_«name»Wrapper.getCommonAPIType();
		}
	'''

	def generateAttributeDefinition(FAttribute attribute) '''
		Q_PROPERTY(«fullyQualifiedQtWrapperName» «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		QtAttributeWrapper<«fullyQualifiedCppName»> m_«attribute.name»AttributeWrapper;
		
		«fullyQualifiedQtWrapperName» get«attribute.name»() {
			return m_«attribute.name»AttributeWrapper.getQtValue();
		 }
	'''

	def generateQMLElementDeclaration(FTypedElement attribute, FTypedElement parent, String attributeIdentifier) '''
		PropertyBox {
		    propertyName: "«attribute.name»"
		    id: «attribute.qmlElementName(parent)»
		    text: (service.availability) ? «attributeIdentifier».toString() : "undefined"
		}
	'''

	def generateQMLAttributeChangeHandler_(FTypedElement attribute, FTypedElement parent, String parentValue) '''
		TODO «attribute.toString»
	'''

	def generateQMLAttributeChangeHandler(FTypedElement attribute, FTypedElement parent, String value) '''
		«attribute.qmlElementName(parent)».propertyValue = «value»;
	'''

	def generateQMLRegisterTypes() '''
		'''

	def generateSignalPropagation(String name) ''''''

//	def String getProxyClassInitializer(FAttribute attribute) {return null}
	def getProxyClassInitializer(FAttribute attribute) '''m_«attribute.name»AttributeWrapper(m_commonAPIProxy.get«attribute.name.toFirstUpper»Attribute())'''

	def getFullyQualifiedQtConstWrapperType() {
		return fullyQualifiedQtWrapperName
	}

}

class CommonAPIOtherQtWrapper extends CommonAPITypeQtWrapper {

	FTypeRef typeRef
	FType type

	new(FTypeRef typeRef) {
		this.typeRef = typeRef
		type = typeRef.derived;
	}

	new(FType type) {
		this.type = type;
	}

	override needsWrapper() {
		return (type != null);
	}

	override getQtNameReference(EObject source) {
		if (type != null)
			return getQtWrapperName(type, source)
		return typeRef.predefined.qtPrimitiveTypeName
	}

	override fullyQualifiedQtWrapperName() {
		if (type != null)
			return getQtWrapperName(type, null)
		return typeRef.predefined.qtPrimitiveTypeName
	}

	override fullyQualifiedCppName() {
		if (type != null) {
			return (type.model.generateCppNamespace + typeRef.getNameReference(type.model));
		} else {
			return getNameReference(typeRef, null);
		}
	}

	override generateAttributeDefinition(FAttribute attribute) '''
		Q_PROPERTY(«fullyQualifiedQtWrapperName» «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		QtAttributeWrapper<«fullyQualifiedCppName»> m_«attribute.name»AttributeWrapper;
		
		«fullyQualifiedQtWrapperName» get«attribute.name»() {
			return 	m_«attribute.name»AttributeWrapper.getQtValue();
		 }
	'''

}

class CommonAPIEnumQtWrapper extends CommonAPIOtherQtWrapper {
	FEnumerationType type

	new(FEnumerationType enumType) {
		super(enumType)
		type = enumType
	}

	override getQtWrapperName_() {
		return type.name
	}

	override needsWrapper() {
		return false
	}

	override generateTypeClassContribution() '''
		enum class «type.name» {
			«FOR enumValue : type.enumerators»
				«enumValue.name» «if(enumValue.value != null) ( ' = ' + enumValue.value)»,
			«ENDFOR»
		};
		
		Q_ENUMS(«type.name»)
	'''

	override getQtNameReference(EObject source) {
		return getQtWrapperName(type, source)
	}

	override fullyQualifiedQtWrapperName() {
		return type.model.generateCppNamespace + type.containingInterface.typeDefinitionClassName + '::' + type.name
	}

	override fullyQualifiedCppName() {
		return generateCppNamespace(type.model) + type.containingInterface.name + '::' + type.name;
	}

	def private fullyQualifiedShadowName(FEnumerationType type) {
		return type.containingInterface.fullyQualifiedTypeDefinitionClassName + '::' + type.name
	}

	override generateDefaultNamespaceContribution() '''
	template<> inline
void assignQtTypeToCommonAPI<«type.fullyQualifiedShadowName», «fullyQualifiedCppName»>(const «type.
		fullyQualifiedShadowName»& qtValue, «fullyQualifiedCppName»& commonAPIValue) {
	commonAPIValue = static_cast<«fullyQualifiedCppName»>(qtValue);
}

template<> struct CommonAPI2QtDataConverter<«fullyQualifiedCppName»> :
CommonAPI2QtEnumConverter<«fullyQualifiedCppName», «type.fullyQualifiedShadowName»>
{
	typedef «type.fullyQualifiedShadowName» QtType;

	CommonAPI2QtDataConverter<«fullyQualifiedCppName»>(const «fullyQualifiedCppName»& commonAPIValue) :
		CommonAPI2QtEnumConverter<«fullyQualifiedCppName», «type.fullyQualifiedShadowName»>(commonAPIValue) {
	}
};

template<> struct CommonAPIDataHolder<«type.fullyQualifiedShadowName»> : CommonAPIDataHolderWithConversion<
		«type.fullyQualifiedShadowName», «fullyQualifiedCppName»> {
	CommonAPIDataHolder(const «type.fullyQualifiedShadowName»& data) :
			CommonAPIDataHolderWithConversion<«type.fullyQualifiedShadowName», «fullyQualifiedCppName»>(data) {
	}
};
	
'''

	override generateQtWrapperClassDefinition(FInterface fInterface) '''

class Const«type.name»QtWrapperByReference: public QtWrapperOfCommonAPIType, public ConstCommonAPIQtDataWrapperByReference<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

public:
	Const«type.name»QtWrapperByReference(const «type.getRelativeNameReference(fInterface.model)»& commonAPIType) :
			ConstCommonAPIQtDataWrapperByReference(commonAPIType) {
	}

};

class «type.name»QtWrapperByReference: public Const«type.name»QtWrapperByReference, public CommonAPIQtDataWrapperByReference<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

    using CommonAPIQtDataWrapperByReference<«type.getRelativeNameReference(fInterface.model)»>::getCommonAPIType;

public:
	«type.name»QtWrapperByReference(«type.getRelativeNameReference(fInterface.model)»& commonAPIType) :
			Const«type.name»QtWrapperByReference(commonAPIType), CommonAPIQtDataWrapperByReference<«type.
		getRelativeNameReference(fInterface.model)»>(commonAPIType) {
	}

    Q_PROPERTY(«type.qtWrapper.fullyQualifiedQtWrapperName» value READ getValue WRITE setValue)

    «type.qtWrapper.fullyQualifiedQtWrapperName» getValue() const {
        return static_cast<«type.qtWrapper.fullyQualifiedQtWrapperName»>(CommonAPIQtDataWrapperByReference<«type.
		getRelativeNameReference(fInterface.model)»>::getCommonAPIType());
    }

    void setValue(«type.qtWrapper.fullyQualifiedQtWrapperName» value) {
        getCommonAPIType() = static_cast<«type.getRelativeNameReference(fInterface.model)»>(value);
    }

};

class «type.name»QtWrapper: public «type.name»QtWrapperByReference, public CommonAPIQtDataWrapper<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

public:
	«type.name»QtWrapper() : «type.name»QtWrapperByReference(CommonAPIQtDataWrapper<«type.
		getRelativeNameReference(fInterface.model)»>::getCommonAPIType()) {
	}

private:

};

	'''

	override generateAttributeDefinition(FAttribute attribute) '''
		Q_PROPERTY(«fullyQualifiedQtWrapperName» «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		QtAttributeWrapper<«fullyQualifiedCppName»> m_«attribute.name»AttributeWrapper;
		
		«fullyQualifiedQtWrapperName» get«attribute.name»() {
			return 	m_«attribute.name»AttributeWrapper.getQtValue();
		 }
	'''

	override generateQMLRegisterTypes() '''
		qmlRegisterType<«getQtNameReference(null)»>(namesp, 1, 0, "«getQtNameReference(null)»");
	'''

}

abstract class CommonAPIQtWrapperWithWrapper extends CommonAPITypeQtWrapper {
	FType type

	new(FType type) {
		this.type = type
	}

	override needsWrapper() {
		return true
	}

	override getQtNameReference(EObject source) {
		return getQtWrapperName(type, source)
	}

	override fullyQualifiedCppName() {
		return (type.model.generateCppNamespace + type.getRelativeNameReference(type.model));
	}

	override generateQtPropertyDefinition(FTypeRef type, String name, FInterface fInterface, boolean byReference,
		boolean isConst, boolean isNotifyable) '''
		Q_PROPERTY(«fInterface.model.generateCppNamespace + 'Const' + type.qtWrapper.getQtNameReference(fInterface.model)»ByReference* «name» READ get«name» «if (isNotifyable) ''' NOTIFY valueChanged'''»);
		«if(isConst) 'Const'»«type.getNameReference(fInterface)»QtWrapper«if(byReference) 'ByReference'» m_«name»Wrapper;
		Const«type.getNameReference(fInterface)»QtWrapperByReference* get«name»() {
			return &m_«name»Wrapper;
		}
		const «type.getNameReference(fInterface.model)»& getCommonAPI«name»() const {
			return m_«name»Wrapper;
		}
	'''

	override fullyQualifiedQtWrapperName() {
		return type.model.generateCppNamespace + getQtWrapperName(type, null)
	}

	def fullyQualifiedConstQtWrapperName() {
		return type.model.generateCppNamespace + "Const"+ getQtWrapperName(type, null)
	}

	override generateQtWrapperClassDefinition(FInterface fInterface) '''
class Const«type.name»QtWrapperByReference: public QtWrapperOfCommonAPIType, public ConstCommonAPIQtDataWrapperByReference<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

public:
	Const«type.name»QtWrapperByReference(const «type.getRelativeNameReference(fInterface.model)»& commonAPIType) :
			ConstCommonAPIQtDataWrapperByReference(commonAPIType) {
	}

};

class «type.name»QtWrapperByReference: public Const«type.name»QtWrapperByReference {

	Q_OBJECT

public:
	«type.name»QtWrapperByReference(«type.getRelativeNameReference(fInterface.model)»& commonAPIType) :
			Const«type.name»QtWrapperByReference(commonAPIType) {
	}

};

class «type.name»QtWrapper: public «type.name»QtWrapperByReference, public CommonAPIQtDataWrapper<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

public:
	«type.name»QtWrapper() : «type.name»QtWrapperByReference(CommonAPIQtDataWrapper<«type.
		getRelativeNameReference(fInterface.model)»>::getCommonAPIType()) {
	}

private:

};

	'''

	def generateAttributeDefinition__(FAttribute attribute) '''
		«fullyQualifiedQtWrapperName» «attribute.name»Value;
		
		Q_PROPERTY(«fullyQualifiedQtWrapperName»ByReference* «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		«fullyQualifiedQtWrapperName»ByReference* get«attribute.name»() {
		 CommonAPI::CallStatus callStatus;
		 auto value = &«attribute.name»Value;
		 m_commonAPIProxy.get«attribute.name.toFirstUpper»Attribute().getValue(callStatus, value->getCommonAPIType_());
		 return value;
		 }
	'''


	def generateAttributeDefinition_(FAttribute attribute) '''
		Q_PROPERTY(«fullyQualifiedQtWrapperName» «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		QtAttributeWrapper<«fullyQualifiedCppName»> m_«attribute.name»AttributeWrapper;
		
		«fullyQualifiedQtWrapperName» get«attribute.name»() {
			return 	m_«attribute.name»AttributeWrapper.getQtValue();
		 }
	'''


	override generateQMLRegisterTypes() '''
		«IF needsWrapper»
			qmlRegisterType<«getQtNameReference(null)»>(namesp, 1, 0, "«getQtNameReference(null)»");
			qmlRegisterUncreatableType<«getQtNameReference(null)»ByReference>(namesp, 1, 0, "«getQtNameReference(null)»", "Message???");
			qmlRegisterUncreatableType<Const«getQtNameReference(null)»ByReference>(namesp, 1, 0, "Const«getQtNameReference(
			null)»ByReference", "Message???");
		«ENDIF»
	'''

	override generateAttributeDefinition(FAttribute attribute) '''
		Q_PROPERTY(Const«fullyQualifiedQtWrapperName»ByReference* «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		QtAttributeWrapper<«fullyQualifiedCppName»> m_«attribute.name»AttributeWrapper;
		
		Const«fullyQualifiedQtWrapperName»ByReference* get«attribute.name»() {
			return m_«attribute.name»AttributeWrapper.getQtValue();
		 }
	'''

}

class CommonAPIStructQtWrapper extends CommonAPIQtWrapperWithWrapper {
	FStructType type

	new(FStructType type) {
		super(type)
		this.type = type
	}

	override generateDefaultNamespaceContribution() '''

template<> struct CommonAPI2QtDataConverter<«fullyQualifiedCppName»> : public CommonAPI2QtDataConverterWithWrapper<«fullyQualifiedCppName»,
		«fullyQualifiedConstQtWrapperName»ByReference, «fullyQualifiedQtWrapperName»ByReference> {

	CommonAPI2QtDataConverter(«fullyQualifiedCppName»& commonAPIType) :
			CommonAPI2QtDataConverterWithWrapper(commonAPIType) {
	}
};

inline QList<QObject*> convertQListForQML(QList<«fullyQualifiedConstQtWrapperName»ByReference*> list) {
	return convertQListToQObjectList(list);
}

'''

	override generateQtWrapperClassDefinition(FInterface fInterface) '''
class Const«type.name»QtWrapperByReference: public QtWrapperOfCommonAPIType, public ConstCommonAPIQtDataWrapperByReference<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

public:
	Const«type.name»QtWrapperByReference(const «type.getRelativeNameReference(fInterface.model)»& commonAPIType) :
			ConstCommonAPIQtDataWrapperByReference<«type.getRelativeNameReference(fInterface.model)»>(commonAPIType)
	        «FOR field : type.elements»
	        , m_«field.name»Wrapper(commonAPIType.«field.name»)
            «ENDFOR»
			{
	}

            «FOR field : type.elements»
			«field.type.qtWrapper.generateQtPropertyDefinition(field.type, field.name, fInterface, true, true, true)»
            «ENDFOR»

Q_SIGNAL virtual void valueChanged();

void triggerOnChanged() {
	valueChanged();
}

};


class «type.name»QtWrapperByReference: public Const«type.name»QtWrapperByReference, public CommonAPIQtDataWrapperByReference<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

public:
	«type.name»QtWrapperByReference(«type.getRelativeNameReference(fInterface.model)»& commonAPIType) :
			Const«type.name»QtWrapperByReference(commonAPIType),
			CommonAPIQtDataWrapperByReference<«type.getRelativeNameReference(fInterface.model)»>(commonAPIType)
	        «FOR field : type.elements»
	        , m_«field.name»Wrapper(commonAPIType.«field.name»)
            «ENDFOR»
			{
	        «FOR field : type.elements»
	        «field.type.qtWrapper.generateSignalPropagation("m_" + field.name + "Wrapper")»
            «ENDFOR»
			}

	~«type.name»QtWrapperByReference() {
		printf("Destructor «type.name»QtWrapperByReference\n");
	}

            «FOR field : type.elements»
	            «field.type.qtWrapper.generateQtPropertyDefinition(field.type, field.name, fInterface, true, false, true)»
            «ENDFOR»

			Q_SIGNAL void valueChanged();

};

class «type.name»QtWrapper: public «type.name»QtWrapperByReference, public CommonAPIQtDataWrapper<«type.
		getRelativeNameReference(fInterface.model)»> {

	Q_OBJECT

public:
	«type.name»QtWrapper() :
			«type.name»QtWrapperByReference(getCommonAPIType_())
	        «FOR field : type.elements»
	        , m_«field.name»Wrapper(getCommonAPIType_().«field.name»)
            «ENDFOR»
			
			
			{
	}

            «FOR field : type.elements»
			«field.type.qtWrapper.generateQtPropertyDefinition(field.type, field.name, fInterface, true, false, true)»
            «ENDFOR»

	Q_SIGNAL void valueChanged();

};


    '''

	override generateQMLElementDeclaration(FTypedElement attribute, FTypedElement parent, String attributeIdentifier) '''
		StructPropertyBox {
		propertyName: "«attribute.name»"
		id: «attribute.qmlElementName(parent)»
		
		«FOR field : type.elements»
			«field.type.qtWrapper.generateQMLElementDeclaration(field, attribute, attributeIdentifier + "." + field.name)»
		«ENDFOR»
		}
	'''

	override generateQMLAttributeChangeHandler(FTypedElement attribute, FTypedElement parent, String value) '''
		«FOR field : type.elements»
			«field.type.qtWrapper.generateQMLAttributeChangeHandler(field, attribute, value + '.' + field.name)»
		«ENDFOR»
	'''

	override generateSignalPropagation(String name) '''
		«name».connectValueChangedEvent(this);
	'''

	override generateAttributeDefinition(FAttribute attribute) '''
		Q_PROPERTY(«fullyQualifiedConstQtWrapperName»ByReference* «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		QtAttributeWrapper<«fullyQualifiedCppName»> m_«attribute.name»AttributeWrapper;
		
		«fullyQualifiedConstQtWrapperName»ByReference* get«attribute.name»() {
			return m_«attribute.name»AttributeWrapper.getQtValue();
		 }
	'''

	override getFullyQualifiedQtConstWrapperType() {
		return "QObject*"
	}

}

class CommonAPIUnionQtWrapper extends CommonAPIQtWrapperWithWrapper {
	FUnionType type

	new(FUnionType type) {
		super(type)
		this.type = type
	}

	override generateQtPropertyDefinition(FTypeRef type, String name, FInterface fInterface, boolean byReference,
		boolean isConst, boolean isNotifyable) '''
				 		Q_PROPERTY(QVariant «name» READ get«name» «if (!isConst) ''' WRITE set«name»'''» «if (isNotifyable) ''' NOTIFY valueChanged'''»);
		«if(isConst) 'Const'»CommonAPIQtDataWrapper«if(byReference) 'ByReference'» <«type.getNameReference(fInterface.model)»> m_«name»Wrapper;
		
		QVariant get«name»() {
			QVariant variant = commonAPIUnionToQtVariant(getCommonAPI«name»());
			return variant;
		}
		
		«if (!isConst) '''
void set«name»(const QVariant value) {
}
 '''»
		
		const «type.getNameReference(fInterface.model)»& getCommonAPI«name»() const {
			return m_«name»Wrapper.getCommonAPIType();
		}
	''' 

//	override generateQMLElementDeclaration(FTypedElement attribute, FTypedElement parent, String attributeIdentifier) '''
//		PropertyBox {
//		propertyName: "«attribute.name»"
//		id: «attribute.qmlElementName(parent)»
//		}
//	'''
}

class CommonAPIArrayQtWrapper extends CommonAPITypeQtWrapper {
FArrayType type

	new(FArrayType type) {
		this.type = type
	}

	override needsWrapper() {
		return false;
	}

	override getQtNameReference(EObject source) {
		return "";
	}
	
	def getListType() '''QList<«type.elementType.qtWrapper.getFullyQualifiedQtConstWrapperType»>'''

	override fullyQualifiedQtWrapperName() {
		return getListType();
	}

	override fullyQualifiedCppName() {
		return (type.model.generateCppNamespace + type.getRelativeNameReference(type.model));
	}

	override generateAttributeDefinition(FAttribute attribute) '''
		Q_PROPERTY(«fullyQualifiedQtWrapperName» «attribute.name» READ get«attribute.name» NOTIFY «attribute.name»Changed);
		
		QtAttributeWrapper<«fullyQualifiedCppName»> m_«attribute.name»AttributeWrapper;
		
		«fullyQualifiedQtWrapperName» get«attribute.name»() {
			return convertQListForQML(m_«attribute.name»AttributeWrapper.getQtValue());
		 }
	'''
	
}



class CommonAPIMapQtWrapper extends CommonAPIQtWrapperWithWrapper {
	FMapType type

	new(FMapType type) {
		super(type)
		this.type = type
	} 
}

class FInterfaceQtProxyGenerator {
	@Inject
	extension FTypeGeneratorMadePublic = new FTypeGeneratorMadePublic
	extension QtGeneratorExtensions = new QtGeneratorExtensions

	def generateQtProxy(FInterface fInterface, IFileSystemAccess fileSystemAccess,
		DeploymentInterfacePropertyAccessor deploymentAccessor) {
		fileSystemAccess.generateFile(fInterface.qtProxyHeaderPath, fInterface.generateProxyHeader)
	}

	def getQtProxyHeaderPath(FInterface fInterface) {
		fInterface.model.directoryPath + '/' + fInterface.name + "QtProxy.h"
	}

	def private generateProxyHeader(FInterface fInterface) '''
        #pragma once

        #include "«fInterface.getProxyHeaderFile»"
        #include <CommonAPI/CommonAPI.h>
//        #include <CommonAPI/Factory.h>

        #include "QtCommonAPIBridge.h"
		#include <QtQml>

        «fInterface.model.generateNamespaceBeginDeclaration»
			class «typeDefinitionClassName(fInterface)» : public QObject {
				Q_OBJECT
			public:
        «FOR type : fInterface.types»
			«type.getQtWrapper.generateTypeClassContribution»
        «ENDFOR»
			};
       	«fInterface.model.generateNamespaceEndDeclaration»

            «FOR type : fInterface.types.sortTypes(fInterface)»
       		«fInterface.model.generateNamespaceBeginDeclaration»
				«type.qtWrapper.generateQtWrapperClassDefinition(fInterface)»
			«fInterface.model.generateNamespaceEndDeclaration»
			«type.qtWrapper.generateDefaultNamespaceContribution»
            «ENDFOR»

        «fInterface.model.generateNamespaceBeginDeclaration»

            «FOR method : fInterface.methods»

class «method.name»Call: public CommonAPIPendingCallResult {

	Q_OBJECT

public:

	«method.name»Call() {
	}

	~«method.name»Call() {
		printf("«method.name»Call instance destroyed\n");
	}

	void onAnswerReceived(const CommonAPI::CallStatus&
	«FOR arg : method.outArgs»
		, const «arg.type.qtWrapper.fullyQualifiedCppName»& «arg.name»
	«ENDFOR»
	) {
	}

	«FOR arg : method.outArgs»
		«arg.type.qtWrapper.generateQtPropertyDefinition(arg.type, arg.name, fInterface, false, false, false)»
	«ENDFOR»


	void callSynchronous(«fInterface.getProxyBaseClassName»& commonAPIProxy
	«FOR arg : method.inArgs»
		, «if (arg.type.qtWrapper.needsWrapper)
			(arg.type.qtWrapper.fullyQualifiedQtWrapperName + '* ' + arg.name)
		else
			('const ' + arg.type.qtWrapper.fullyQualifiedQtWrapperName + '& ' + arg.name)»
	«ENDFOR»
	) {
		
            «FOR arg : method.inArgs»
            CommonAPIDataHolder<«arg.type.qtWrapper.fullyQualifiedQtWrapperName»> wrapperInputParam_«arg.name»( «(if(getQtWrapper(
		arg.type).needsWrapper) '*' else '') + arg.name»);
            «ENDFOR»
 
        commonAPIProxy.«method.name»(«ListBuilder::join(
		method.inArgs.map[(if(!getQtWrapper(type).needsWrapper) 'wrapperInputParam_' else '*') + name],
		'getCallStatus()', method.outArgs.map['m_' + name + 'Wrapper.getCommonAPIType_()']).
		join(', ')» );

	}

	void callAsynchronous(«fInterface.getProxyBaseClassName»& commonAPIProxy
	«FOR arg : method.inArgs»
		, «if (arg.type.qtWrapper.needsWrapper)
			(arg.type.qtWrapper.fullyQualifiedQtWrapperName + '* ' + arg.name)
		else
			('const ' + arg.type.qtWrapper.fullyQualifiedQtWrapperName + '& ' + arg.name)»
	«ENDFOR»
	, QJSValue callBackFunction) {
		m_callbackJSFunction = callBackFunction;
            «FOR arg : method.inArgs»
            CommonAPIDataHolder<«arg.type.qtWrapper.fullyQualifiedQtWrapperName»> wrapperInputParam_«arg.name»( «(if(getQtWrapper(
		arg.type).needsWrapper) '*' else '') + arg.name»);
            «ENDFOR»

        commonAPIProxy.«method.name»Async(«ListBuilder::join(
		method.inArgs.map[(if(!getQtWrapper(type).needsWrapper) 'wrapperInputParam_' else '*') + name]).join(', ')»
		«if (method.inArgs.size > 0) ","» [&](const CommonAPI::CallStatus& status
            «FOR arg : method.outArgs»
            , const «arg.type.qtWrapper.fullyQualifiedCppName»& «arg.name»
            «ENDFOR»
) {
	onCallFinished();
	printf("«fInterface.getProxyBaseClassName» Done\n");
}
		 );

	}

		friend class «fInterface.name»QtProxy;

};

            «ENDFOR»



        class «fInterface.qtProxyClassName» : public QtCommonAPIProxy {

		LOG_DECLARE_CLASS_CONTEXT("«fInterface.name»", "«fInterface.name» Qt wrapper");

		Q_OBJECT

	    «FOR type : fInterface.types.sortTypes(fInterface)»
	          «if (type instanceof FEnumerationType) {
		var enumType = type as FEnumerationType
		'''
		Q_ENUMS(«enumType.qtWrapper.fullyQualifiedQtWrapperName»)

'''
	}»
		«ENDFOR»


		«fInterface.getProxyBaseClassName»& m_commonAPIProxy;

        public:

			«fInterface.getProxyBaseClassName»& getProxy()	{
				return m_commonAPIProxy;
			}
			
            «fInterface.qtProxyClassName»(«fInterface.getProxyBaseClassName»& proxy) : QtCommonAPIProxy(proxy), m_commonAPIProxy(proxy)
	«var myList = new ArrayList()»
	«{
			for (attr : fInterface.attributes) {
				if (attr.type.qtWrapper.getProxyClassInitializer(attr) != null)
					myList.add(attr.type.qtWrapper.getProxyClassInitializer(attr))
			}
	}»

« if (myList.size() !=0) ''', «myList.join(',\n')»'''»

             {
            }

            ~«fInterface.qtProxyClassName»() {
            }

            void triggerAttributeChangedSignals() override {
			«FOR attribute : fInterface.attributes»
				«attribute.name»Changed();
			«ENDFOR»
           	}

    void subscribe(const QMetaMethod &signal) override {
           «FOR attribute : fInterface.attributes»

        if (signal == QMetaMethod::fromSignal(&«fInterface.qtProxyClassName»::«attribute.name»Changed))
        	m_«attribute.name»AttributeWrapper.subscribe(
					QMetaMethod::fromSignal(&«fInterface.qtProxyClassName»::«attribute.name»Changed), this);
           «ENDFOR»

           «FOR broadcast : fInterface.broadcasts»

        if (signal == QMetaMethod::fromSignal(&«fInterface.qtProxyClassName»::«broadcast.name.toFirstLower»)) {
        m_commonAPIProxy.get«broadcast.name.toFirstUpper»Event().subscribe([&](«broadcast.outArgs.map[
		'const ' + type.getNameReference(fInterface.model) + '& ' + name].join(', ')») {

            «FOR arg : broadcast.outArgs»
            	«if (getQtWrapper(arg.type).needsWrapper) '''Const«arg.type.qtWrapper.getQtNameReference(arg.model)»ByReference wrapperParam_«arg.name»(«arg.name»); ''' else '''BroadcastArgumentWrapper<«arg.type.getNameReference(arg.model)»> wrapperParam_«arg.name»(«arg.name»); '''»

            «ENDFOR»

               	«broadcast.name.toFirstLower»(
                	«broadcast.outArgs.map[(if(getQtWrapper(type).needsWrapper) '&' else '') + 'wrapperParam_' + name + ''].
		join(', ')»
                	);
            	});
        }

            «ENDFOR»
			}

                        «FOR attribute : fInterface.attributes»

                        Q_SIGNAL void «attribute.name.toFirstLower»Changed();

						«attribute.type.qtWrapper.generateAttributeDefinition(attribute)»
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
            Q_SIGNAL void «broadcast.name.toFirstLower»(
         «broadcast.outArgs.map[
		if (getQtWrapper(type).needsWrapper)
			( 'Const' + type.qtWrapper.getQtNameReference(broadcast.model) + 'ByReference* ' + name)
		else
			('const ' + type.qtWrapper.fullyQualifiedQtWrapperName + '& ' + name)].join(', ')»
            );
            «ENDFOR»


            «FOR method : fInterface.methods»

                /**
                 * Calls «method.name» with «IF method.isFireAndForget»Fire&Forget«ELSE»synchronous«ENDIF» semantics.
                 *
«IF !method.inArgs.empty»     * All const parameters are input parameters to this method.«ENDIF»
«IF !method.outArgs.empty»     * All non-const parameters will be filled with the returned values.«ENDIF»
                 * The CallStatus will be filled when the method returns and indicate either
                 * "SUCCESS" or which type of error has occurred. In case of an error, ONLY the CallStatus
                 * will be set.
                 */
                Q_INVOKABLE «method.model.generateCppNamespace + method.name»Call* «method.name»(«ListBuilder::join(method.inArgs.map[
		if (type.qtWrapper.needsWrapper)
			(type.qtWrapper.fullyQualifiedQtWrapperName + '* ' + name)
		else
			('const ' + type.qtWrapper.fullyQualifiedQtWrapperName + '& ' + name)], "QJSValue callBackJSFunction").join(', ')»)
                {
        «method.name»Call* returnValue = new «method.name»Call();

            «FOR arg : method.inArgs»
            CommonAPIDataHolder<«arg.type.qtWrapper.fullyQualifiedQtWrapperName»> wrapperInputParam_«arg.name»( «(if(getQtWrapper(
		arg.type).needsWrapper) '*' else '') + arg.name»);
            «ENDFOR»
 
 		returnValue->callAsynchronous(m_commonAPIProxy
            «FOR arg : method.inArgs»
            	, «arg.name»
            «ENDFOR»
            , callBackJSFunction
      );

        return returnValue;
     }

                /**
                 * Calls «method.name» with «IF method.isFireAndForget»Fire&Forget«ELSE»synchronous«ENDIF» semantics.
                 *
«IF !method.inArgs.empty»     * All const parameters are input parameters to this method.«ENDIF»
«IF !method.outArgs.empty»     * All non-const parameters will be filled with the returned values.«ENDIF»
                 * The CallStatus will be filled when the method returns and indicate either
                 * "SUCCESS" or which type of error has occurred. In case of an error, ONLY the CallStatus
                 * will be set.
                 */
                Q_INVOKABLE «method.model.generateCppNamespace + method.name»Call* «method.name»(«ListBuilder::join(method.inArgs.map[
		if (type.qtWrapper.needsWrapper)
			(type.qtWrapper.fullyQualifiedQtWrapperName + '* ' + name)
		else
			('const ' + type.qtWrapper.fullyQualifiedQtWrapperName + '& ' + name)]).join(', ')»)
                {
                	return «method.name»(«ListBuilder::join(method.inArgs.map[name], "QJSValue()").join(', ')»);
     }

            «ENDFOR»

	            «FOR type : fInterface.types.sortTypes(fInterface)»
	          «if (type instanceof FStructType) '''
Q_INVOKABLE «type.qtWrapper.fullyQualifiedQtWrapperName»* create«type.name»() {
	return new «type.name»QtWrapper();
}
'''»
				«ENDFOR»

#ifndef QML_ENABLED
	static void registerQMLTypes() {
		
		const char* namesp = "«fInterface.model.name».constants";

			// TODO : find a better way to get the enums registered 
		    qmlRegisterUncreatableType<«fInterface.name»QtProxy>(namesp, 1, 0, "«fInterface.name»Constants",  "Message???");

	            «FOR type : fInterface.types.sortTypes(fInterface)»
	            «type.qtWrapper.generateQMLRegisterTypes»
				«ENDFOR»

	            «FOR method : fInterface.methods»
	            qmlRegisterUncreatableType<«method.name»Call>(namesp, 1, 0, "«method.name»Call", "Message???");
				«ENDFOR»
	}
#endif

};
}

        «FOR type : fInterface.types»
        Q_DECLARE_METATYPE(«type.model.generateCppNamespace + type.getRelativeNameReference(type.model)»)
        «ENDFOR»

    '''

	def generateQmlUI(FInterface fInterface, IFileSystemAccess fileSystemAccess,
		DeploymentInterfacePropertyAccessor deploymentAccessor) {

		val basePath = fInterface.model.directoryPath + '/';

		fileSystemAccess.generateFile(basePath + fInterface.name + "UI.qml", fInterface.generateQMLUI)
		fileSystemAccess.generateFile(basePath + "qmldir", fInterface.generateQMLPluginFile)
		fileSystemAccess.generateFile(basePath + fInterface.name + "QMLPlugin.cpp", fInterface.generateQMLPluginCppFile)
		fileSystemAccess.generateFile(basePath + "plugin.json", '{"keys": [ ]}')
	}

	def private generateQMLPluginFile(FInterface fInterface) '''
		module «fInterface.model.name»
		«fInterface.name»UI 1.0 «fInterface.name»UI.qml
		plugin «fInterface.name»
	'''

	def private generateQMLPluginCppFile(FInterface fInterface) '''
#include "«fInterface.model.directoryPath»/«fInterface.name»QtProxy.h"
#include "«fInterface.model.directoryPath»/«fInterface.name»Proxy.h"

#include "QtWrapperQMLPlugin.h"

typedef QtWrapperQMLPlugin<«fInterface.fullyQualifiedCppName»Proxy<>, «fInterface.fullyQualifiedCppName»QtProxy> Qml«fInterface.
		name»PluginBaseClass;

class Qml«fInterface.name»Plugin: public Qml«fInterface.name»PluginBaseClass {

	Q_OBJECT
	Q_PLUGIN_METADATA(IID "«fInterface.model.name»" FILE "plugin.json")

public:

	Qml«fInterface.name»Plugin() : Qml«fInterface.name»PluginBaseClass("«fInterface.model.name»", "«fInterface.name.
		toFirstUpper»") {
	}

	std::shared_ptr< «fInterface.fullyQualifiedCppName»Proxy<> > createProxyInstance(GLibCommonAPIFactory& factory) override {
//		return factory.buildProxy<«fInterface.fullyQualifiedCppName»Proxy>(address.toStdString());
		return factory.buildProxy<«fInterface.fullyQualifiedCppName»Proxy>();
	}

};

template<> Qml«fInterface.name»PluginBaseClass::Backend* Qml«fInterface.name»PluginBaseClass::backend = NULL;

#include "«fInterface.name»QMLPluginmoc.cpp"
	'''

	def private generateQMLUI(FInterface fInterface) '''

import QtQuick 2.0
import com.pelagicore.testui 1.0

Rectangle {
    height: childrenRect.height
    width : childrenRect.width
    color: "#808080"
	property var service

    PropertiesColumn {

		propertyName : "«fInterface.name»"
    id: servicePanel

           «FOR attribute : fInterface.attributes»
				«attribute.type.qtWrapper.generateQMLElementDeclaration(attribute, null, "service" + '.' + attribute.name)»
           «ENDFOR»

    }

    Connections {
        target: service

«««           «FOR attribute : fInterface.attributes»
«««        on«attribute.name.toFirstUpper»Changed: {
«««			«attribute.type.qtWrapper.generateQMLAttributeChangeHandler(attribute, null,
«««		attribute.containingInterface.name.toFirstLower + '.' + attribute.name)»
«««		}
«««           «ENDFOR»

        onAvailabilityChanged: {
            servicePanel.enabled = service.availability;

//            if (service.availability) {
//                servicePanel.color = "#8080C0";
//            } else {
//                servicePanel.color = "#101010";
//            }

        }
    }

}

    '''

}
