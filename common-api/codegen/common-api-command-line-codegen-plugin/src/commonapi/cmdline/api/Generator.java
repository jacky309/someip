package commonapi.cmdline.api;

import java.util.List;

import org.eclipse.core.resources.IResource;
import org.eclipse.xtext.generator.IFileSystemAccess;
import org.franca.core.franca.FModel;
import org.franca.deploymodel.dsl.fDeploy.FDInterface;
import org.genivi.commonapi.core.generator.FrancaGenerator;

import com.google.inject.Inject;

import commonapi.cmdline.GeneratorInterface;

public class Generator implements GeneratorInterface {

	@Inject
	FrancaGenerator generator;

	public void generate(FModel fModel, List<FDInterface> deployedInterfaces,
			IFileSystemAccess fileSystemAccess, IResource res) {
		generator.doGenerateComponents(fModel, deployedInterfaces,
				fileSystemAccess, res);
	}

}
