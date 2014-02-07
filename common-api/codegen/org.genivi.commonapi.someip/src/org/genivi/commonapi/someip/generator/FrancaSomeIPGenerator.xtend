package org.genivi.commonapi.someip.generator

import javax.inject.Inject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.eclipse.xtext.generator.IGenerator
import org.franca.core.dsl.FrancaPersistenceManager
import org.genivi.commonapi.core.generator.FrancaGenerator
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions

import static com.google.common.base.Preconditions.*
import org.franca.core.franca.FModel
import java.util.List
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import java.util.LinkedList
import org.franca.deploymodel.dsl.FDeployPersistenceManager
import org.franca.deploymodel.core.FDModelExtender

class FrancaSomeIPGenerator implements IGenerator {
    @Inject private extension FrancaGeneratorExtensions
    @Inject private extension FInterfaceSomeIPProxyGenerator
    @Inject private extension FInterfaceSomeIPStubAdapterGenerator
    @Inject private extension FrancaGenerator

    @Inject private FrancaPersistenceManager francaPersistenceManager
    @Inject private FDeployPersistenceManager fDeployPersistenceManager
    @Inject private FrancaGenerator francaGenerator

    override doGenerate(Resource input, IFileSystemAccess fileSystemAccess) {
        var FModel fModel
        var List<FDInterface> deployedInterfaces

        if(input.
        	URI.
        	fileExtension.
        	equals(francaPersistenceManager.
        		fileExtension
        	)
        ) {
            francaGenerator.doGenerate(input, fileSystemAccess);
            fModel = francaPersistenceManager.loadModel(input.filePath)
            deployedInterfaces = new LinkedList<FDInterface>()

        } else if (input.URI.fileExtension.equals("fdepl" /* fDeployPersistenceManager.fileExtension */)) {
            francaGenerator.doGenerate(input, fileSystemAccess);

            var fDeployedModel = fDeployPersistenceManager.loadModel(input.filePathUrl);
            val fModelExtender = new FDModelExtender(fDeployedModel);

            checkArgument(fModelExtender.getFDInterfaces().size > 0, "No Interfaces were deployed, nothing to generate.")
            fModel = fModelExtender.getFDInterfaces().get(0).target.model
            deployedInterfaces = fModelExtender.getFDInterfaces()

	        deployedInterfaces.forEach[
				var currentInterface = it
			]

        } else {
            checkArgument(false, "Unknown input: " + input)
        }

        doGenerate(deployedInterfaces, fileSystemAccess)
    }

    def public doGenerate(List<FDInterface> deployedInterfaces, IFileSystemAccess fileSystemAccess) {
        deployedInterfaces.forEach[
			var currentInterface = it
			generateSomeIPProxy(currentInterface, fileSystemAccess)
			generateSomeIPStubAdapter(currentInterface, fileSystemAccess)
		]
    }

}