/**

\page LowLevelClientLibraryDocPage Low-level client library
The low-level client library should be used by applications to connect to the dispatcher and interact with it.
Applications normally only use the API defined by CommonAPI (http://projects.genivi.org/commonapi) in order to remain agnostic from the IPC technology used to actually transport the data.

\section Requirements
Here are the main requirements fulfilled by the client library:
        \li The library should support blocking and non-blocking calls.
        \li Integration into GLib main loop should be supported.

\section Design
The library is mainly made of the following classes
        \li SomeIPClient::ClientConnection
        \li SomeIPClient::GLibIntegration

\dot
digraph G {
layout=fdp;
style=filled;
color=lightgrey;
overlap=false;
maxiter = 1070;
ranksep=100.5;
splines = true;

        subgraph cluster_ClientConnection {
                style=filled;
                color=lightgrey;
                node [style=filled,color=white];
                ClientConnection [label = "ClientConnection"];
                label = "SomeIP low-level client library";
        }

        subgraph cluster_GLib {
                style=filled;
                color=red;
                node [style=filled,color=white];
                GLib_MainLoop [label = "Main Loop"];
                label = "GLib";
        }

        subgraph cluster_GlibMainLoopIntegration {
                style=filled;
                color=red;
                node [style=filled,color=white];
                GlibMainLoopIntegration [label = "Glib main loop integration"];
                label = "GlibMainLoopIntegration";
        }

        GlibMainLoopIntegration -> GLib_MainLoop;
        GlibMainLoopIntegration -> ClientConnection;
}
\enddot

\section Dependencies
        \li GLib

\section FAQ FAQ
\li TODO

\section ChangeLog Change log
\include ChangeLog.txt

*/
