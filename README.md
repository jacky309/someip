My Main Page                         {#mainpage}
============

Introduction
------------

This documentation describes the Some/IP dispatcher and the related components.
The purpose of this software is to provide a mean for applications to communicate:

- With each other using a fast local IPC (Inter Process Communication)
- With external devices using the SomeIP protocol. See http://some-ip.com


Applications normally interact with the software using an API defined by the CommonAPI project (http://projects.genivi.org/commonapi), but they can also directly use the low-level API if CommonAPI is not suitable.
Providing an interface based on CommonAPI means client applications (at least most parts of them) likely have no direct dependency to our package, but only to the CommonAPI package, which means current applications based on CommonAPI can very easily be changed to use our backend instead of any existing CommonAPI backend such as DBUS.

Using the SomeIP backend instead of DBUS for local communication is likely to bring a significant performance improvement, especially for applications exchanging small messages at a high frequency. The performance gain would be mainly due to the following factors:

- Simplicity of SomeIP's wire protocol.
- Optimization of the serialization and deserialization

In addition to the performance improvements, this package offers you the possibility to interact with external devices, using the Some/IP protocol, which is specified in AUTOSAR (4.1).


Dependencies
------------

The following packages need to be installed in order to be able to compile SomeIP
- glib 2
- GDBus (for systemD support)
- maven (code generator)
- PLog
- unzip
- wget

On Ubuntu or debian, those dependencies can be installed with the following command: 
	$ sudo apt-get install libglib2.0-dev maven libdbus-glib-1-dev unzip

// TODO : add location of Plog 

Build
-----

Here are instructions on how to build the package:
- 


Content
-------

This package is made of the following components:

- \ref DispatcherDocPage
- \ref LowLevelClientLibraryDocPage
- \ref CommonAPIDocPage

\tableofcontents

FAQ
---

- TODO


TODO
----

- Validate protocol implementation against Some/IP specification
- Add security features
- Implement IPv6 support

Possibility to configure a local "subnet" in order to include only selected hosts in the service discovery process.
Implement more unit tests

Write documentation.
Submit CommonAPI patches.

- 
- ...


Change log
----------

\include ChangeLog.txt

