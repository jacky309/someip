
Dependencies
------------

The following packages need to be installed in order to be able to compile SomeIP
- glib 2
- GDBus (for systemD support)
- maven (for the code generator)
- unzip
- wget

On Ubuntu or debian, those dependencies can be installed with the following command: 
	$ sudo apt-get install libglib2.0-dev maven libdbus-glib-1-dev unzip

Additionally, you need to download those packages:
- PLog (TODO : URL) git@git.pelagicore.de:development-tools/pelagicore-logging.git
- git@git.pelagicore.de:ipc/org-genivi-commonapi-cmdline.git (TODO : URL)
- git@git.pelagicore.de:jacques.guillou/common-api-tools.git (TODO : URL)

Build
-----

Here are instructions on how to build the package:
- Download and install the "org-genivi-commonapi-cmdline" package, which contains the runtime needed for commonAPI command-line generators.
	$ git clone git@git.pelagicore.de:ipc/org-genivi-commonapi-cmdline.git (TODO use public address)
	$ cd org-genivi-commonapi-cmdline
	$ cmake -DCMAKE_INSTALL_PREFIX=/My/Installation/Location
	$ make install
- Download and install the "common-api-tools" component, and build the "org.genivi.commonapi.core" package, which is needed by the Some/IP generator to generate the API of your stubs/proxies
	$ git clone git@git.pelagicore.de:jacques.guillou/common-api-tools.git (TODO use public address)
	$ cd common-api-tools/org.genivi.commonapi.core
	$ mvn install
- Build and install our package
	$ cd someip
	$ cmake . -DCMAKE_INSTALL_PREFIX=/My/Installation/Location
	$ make install
- Build and install the CMake integration package
	$ git clone git@git.pelagicore.de:ipc/common-api-command-line-codegen.git
	$ cd common-api-command-line-codegen
	$ cmake . -DCMAKE_INSTALL_PREFIX=/My/Installation/Location
	$ make install
- Example application
	$ git clone git@git.pelagicore.de:jacques.guillou/common-api-someip-test-app.git
	$ cd common-api-someip-test-app
	$ cmake . -DCMAKE_INSTALL_PREFIX=/My/Installation/Location
	$ make install


