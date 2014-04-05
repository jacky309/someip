/**

\page DispatcherDocPage Dispatcher daemon
Dispatcher daemon

\section Dispatcher
The dispatcher is a process, which is in charge of handling the requests coming from clients. 2 types of clients exist:
        \li Applications running locally. The communication between the client processes and the dispatcher is using a local IPC channel (Unix Domain Socket).
        \li Devices connected via TCP or UDP (UDP not yet implemented).

\subsection Requirements
Here are the main requirements fulfilled by the dispatcher:
        \li In case of a congestion due to a slow client, the dispatcher should keep on dispatching messages which are not involving the client causing the congestion
        \li Similarly to DBUS, the dispatcher should support on-demand service activation. The services should be described in configuration files read by the dispatcher on startup, and should be immediately registered. A service should be actually started as soon as a client makes a request to it.
        \li On platforms using SystemD, the dispatcher should support SystemD's socket activation.
        \li On platforms using SystemD, the dispatcher should activate services via SystemD.

\subsection Design
Here are the main components:
        \li Dispatcher core. This component handle the core features such as the registration/unregistration of services and the message dispatching from one client to another.
        \li Local Server. This component handles the connection of the local client applications. Unix domain sockets are currently used as low-level IPC channel.
        \li TCP Server. This component handles the connection of client applications via TCP.
        \li Service announcer. This component is in charge of sending notifications on the network (via UDP broadcasts) as soon as a service has been registered or unregistered.
        \li Remote service listener. This component listens to notifications sent by other devices on the network and registers those service locally, so that they can be used by local clients.

\dot
digraph G {
layout=fdp;
color=lightgrey;
overlap=false;
maxiter = 130;
ranksep=100.5;
splines = true;
style=filled;
node [shape=box, style=filled,color=lightgrey];

        subgraph cluster_SomeIP_Dispatcher {
                label = "SomeIP dispatcher";
                color=grey;
                Dispatcher [label = "Dispatcher core"];
                LocalServer [label = "Local Server"];
                TCPServer [label = "TCP Server"];
                ServiceActivator [label = "Service activator"];
                RemoteServiceListener [label = "Remote service listener"];
                ServiceAnnouncer [label = "Service announcer"];
        }

        subgraph cluster_Linux {
                label = "Linux";
                color=blue;
                TCPIPStack [label = "TCP/IP Stack"];
                UDS [label = "Unix domain socket"];
        }

        subgraph cluster_SystemD {
                label = "SystemD";
                color=red;
                SystemD_DBUS_Service [label = "DBUS service"];
        }

        subgraph cluster_GLib {
                label = "GLib";
                color=red;
                GLib_MainLoop [label = "Main Loop"];
        }

        subgraph cluster_DBUS {
                color=red;
                DBUS_lib [label = "Library"];
                label = "DBUS";
        }

        LocalServer -> Dispatcher;
        LocalServer -> UDS;
        TCPServer -> Dispatcher;
        TCPServer -> TCPIPStack;
        RemoteServiceListener -> TCPIPStack;
        RemoteServiceListener -> Dispatcher;
        ServiceActivator -> Dispatcher;
        ServiceActivator -> SystemD_DBUS_Service [style="dotted", label = "via DBUS"];
        ServiceActivator -> DBUS_lib;
        ServiceAnnouncer -> Dispatcher;
        ServiceAnnouncer -> TCPIPStack;
        Dispatcher -> GLib_MainLoop;
}
\enddot


\subsection Dependencies
        \li GLib
        \li DBUS-glib
        \li SystemD (optional)


\section FAQ FAQ
\li TODO

\section ChangeLog Change log
\include ChangeLog.txt

*/
