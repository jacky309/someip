package org.genivi.commonapi.someip.generator

import org.franca.core.franca.FModelElement
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions
import javax.inject.Inject
import org.genivi.commonapi.core.generator.FTypeGenerator
import org.eclipse.emf.common.util.EList
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.genivi.commonapi.core.generator.FTypeCycleDetector
import java.util.LinkedList
import java.util.List
import static com.google.common.base.Preconditions.*

/**
 * This class contains public versions of some private methods defined in  FTypeGenerator
 * TODO : Check how the methods from FTypeGenerator could be made public in order to avoid code duplication
 */
class FTypeGeneratorMadePublic extends FTypeGenerator {
	@Inject private extension FrancaGeneratorExtensions francaGeneratorExtensions
	
    def getClassNamespace(FModelElement child, FModelElement parent) {
        child.getClassNamespaceWithName(child.elementName, parent, parent.elementName)
    }

    def private getClassNamespaceWithName(FModelElement child, String name, FModelElement parent, String parentName) {
        var reference = name
        if (parent != null && parent != child)
            reference = parentName + '::' + reference
        return reference
    }
	
   	def sortTypes(EList<FType> typeList, FTypeCollection containingTypeCollection) {
        checkArgument(typeList.hasNoCircularDependencies, 'FTypeCollection or FInterface has circular dependencies: ' + containingTypeCollection)
        var LinkedList<FType> sortedTypeList = new LinkedList<FType>()
        for(currentType: typeList) {
            var insertIndex = 0
            while(insertIndex < sortedTypeList.size && !currentType.isReferencedBy(sortedTypeList.get(insertIndex))) {
                insertIndex = insertIndex + 1
            }
            sortedTypeList.add(insertIndex, currentType)
        }
        return sortedTypeList
    }
        
    def private hasNoCircularDependencies(EList<FType> types) {
        val cycleDetector = new FTypeCycleDetector(francaGeneratorExtensions)
        return !cycleDetector.hasCycle(types)
    }

    def private isReferencedBy(FType lhs, FType rhs) {
        return rhs.referencedTypes.contains(lhs)
    }
    
    def private List<FType> getReferencedTypes(FType fType) {
        var references = fType.directlyReferencedTypes
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }
    
	
}