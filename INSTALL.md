
Dependencies
------------

The following packages need to be installed in order to be able to compile SomeIP
- glib 2
- GDBus (for systemD support)
- maven (for the code generator)
- unzip
- wget
- boost

On Ubuntu or debian, those dependencies can be installed with the following command: 
	$ sudo apt-get install libglib2.0-dev maven libdbus-glib-1-dev unzip libboost-dev 

Additionally, you need to download those packages:
- Common API 2.1.4 or API 2.1.6 : http://git.projects.genivi.org/ipc/common-api-runtime.git
- IVI-Log : https://github.com/Pelagicore/ivi-logging.git
- Common API command line generator launcher : https://github.com/Pelagicore/common-api-cmdline.git
- Common API Tools fork : https://github.com/Pelagicore/common-api-tools.git



Build
-----

Here are instructions on how to build the package:
- Download and install the "common-api-cmdline" package, which contains the runtime needed for commonAPI command-line generators.
	$ git clone https://github.com/Pelagicore/common-api-cmdline.git
	$ cd common-api-cmdline
	$ cmake -DCMAKE_INSTALL_PREFIX=/My/Installation/Location
	$ make install
- Download and install the "common-api-tools" component, and build the "org.genivi.commonapi.core" package, which is needed by the Some/IP generator to generate the API of your stubs/proxies
	$ git clone https://github.com/Pelagicore/common-api-tools.git
	$ cd common-api-tools/org.genivi.commonapi.core
	$ mvn install
- Build and install our package
	$ cd someip
	$ cmake . -DCMAKE_INSTALL_PREFIX=/My/Installation/Location
	$ make install


Build examples
--------------

Follow these steps to build the example application:
- Build and install the CMake integration package
	$ git clone https://github.com/Pelagicore/common-api-cmake.git
	$ cd common-api-command-line-codegen
	$ cmake . -DCMAKE_INSTALL_PREFIX=/My/Installation/Location
	$ make install
- Example application
	$ git clone https://github.com/Pelagicore/common-api-someip-test-app.git
	Check the README file of that repository for installation instructions


Starting the example application
--------------------------------

Please check the README file from the example repository : https://raw.githubusercontent.com/Pelagicore/common-api-someip-test-app/master/README.md
