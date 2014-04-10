/**

\mainpag SomeIP

\section Introduction

\include "README.md"

This documentation describes the Some/IP dispatcher and the related components.
The purpose of this software is to provide a mean for applications to communicate:
        \li With each other using a fast local IPC (Inter Process Communication).
        \li With external devices using the SomeIP protocol. See http://some-ip.com

Applications normally interact with the software using an API defined by the CommonAPI project (http://projects.genivi.org/commonapi), although they can directly use the low-level specific API.
Providing an interface based on CommonAPI means client applications (at least most parts of them) likely have no direct dependency to our package, but only to the CommonAPI package, which means current applications based on CommonAPI can very easily be changed to use our backend instead of any existing CommonAPI backend such as DBUS.<p>
Using the SomeIP backend instead of DBUS for local communication is likely to bring a significant performance improvement, especially for applications exchanging small messages at a high frequency. The performance gain would be mainly due to the following factors:
        \li Simplicity of SomeIP's wire protocol.
        \li Optimization of the serialization and deserialization

In addition to the performance improvements, this package offers you the possibility to interact with external devices, using the Some/IP protocol, which is specified in AUTOSAR (4.1).

\section Content
This package is made of the following components:
        \li \ref DispatcherDocPage
        \li \ref LowLevelClientLibraryDocPage
        \li \ref CommonAPIDocPage

\tableofcontents

\include "Dispatcher_docpage.h"

\section FAQ FAQ
\li TODO

\section TODO TODO
\li Validate protocol implementation against Some/IP specification
\li Add security features
\li Implement IPv6 support

Possibility to configure a local "subnet" in order to include only selected hosts in the service discovery process.
Implement more unit tests

Write documentation.
Submit CommonAPI patches.

\li
\li ...

\section ChangeLog Change log
\include ChangeLog.txt

*/
