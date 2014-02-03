/**
\page CommonAPIDocPage CommonAPI backend
This package is made of the following components:
        \li A code generator producing stubs and proxies compliant with the CommonAPI interface and using SomeIP as underlying transport technology.
        \li A library providing the classes which are needed by the code produced by the code generator.

\tableofcontents

\section Requirements Requirements
Here are the main requirements:
        \li Produce artifacts (stubs and proxies) which are compliant with the CommonAPI interfaces.
        \li Support all the features and data types defined by CommonAPI.
        \li Use SomeIP as IPC channel.

\section Design
TODO

\dot
digraph G {
layout=fdp;
style=filled;
color=lightgrey;
overlap=false;
maxiter = 100;
ranksep=100.5;
splines = true;

        subgraph cluster_ClientConnection {
                style=filled;
                color=lightgrey;
                node [style=filled,color=white];
                ClientConnection [label = "ClientConnection"];
                label = "SomeIP low-level client library";
        }

        subgraph cluster_CommonAPISomeIPLib {
                style=filled;
                color=lightgrey;
                node [style=filled,color=white];
                CommonAPISomeIPLib [label = "CommonAPI/SomeIP backend"];
                label = "CommonAPI/SomeIP library";
        }

        subgraph cluster_CommonAPI {
                style=filled;
                color=red;
                node [style=filled,color=white];
                CommonAPI [label = "CommonAPI"];
                label = "CommonAPI";
        }

        CommonAPISomeIPLib -> CommonAPI;
        CommonAPISomeIPLib -> ClientConnection;
}
\enddot

\subsection ExternalDependencies External dependencies
        \li CommonAPI
        \li SomeIP client library

\section ServiceDescription Service description

Here is an example of service description in the Franca IDL format :
\include "TestInterface.fidl"

\section CodeGeneration Code generation

In order to completely define the parameters needed for a communication using the SomeIP protocol, some additional parameters need to be defined for each service:
        \li Service ID
        \li Address of the device providing the service. This parameter is optional.

This information is stored in a deployment file such as the following:
\include "AllTest.fdepl"

This deployment file is normally used as input by the code generator. The code generator will generate all the artifacts (stubs and proxies) which are referred in the deployment file.

\section CodeGenerationExample Example of generated code
The code generation produces several artifacts:
        \li CommonAPI headers and source files. These files contain code which is not dependent on the actual IPC technology used. They are mostly interface definitions.
        \li SomeIP headers and source files. These files contain the implementation code needed to provide (stub classes) or use (proxy classes) a service over SomeIP.

Example of proxy file:
\include "TestInterfaceSomeIPProxy.h"
\include "TestInterfaceSomeIPProxy.cpp"

Example of stub adapter file:
\include "TestInterfaceSomeIPStubAdapter.h"
\include "TestInterfaceSomeIPStubAdapter.cpp"

\section FAQ FAQ
\li TODO

\section ChangeLog Change log
\include ChangeLog.txt

*/
