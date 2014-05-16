Main Page                         {#mainpage}
============

Introduction
------------

This documentation describes the Some/IP dispatcher and the related components.
The purpose of this software is to provide a mean for applications to communicate:
- With external devices using the SomeIP protocol, especially devices running AUTOSAR 4.1. See http://some-ip.com
- With each other using a fast local IPC (Inter Process Communication)


Applications normally interact with the software using an API defined by the CommonAPI project (http://projects.genivi.org/commonapi), but they can also directly 
use the low-level API if CommonAPI is not suitable.
By providing an interface based on CommonAPI, this software enables client applications using CommonAPI to be very easily adapted to use the Some/IP backend instead
of any other backend such as DBUS.

Used as local IPC, Some/IP is likely to provide better performance than DBUS in many situations which are relevant for automotive (especially small high frequency messages), due to the following:
- Simplicity of SomeIP's wire protocol
- High dispatching efficiency

Content
-------

This package is made of the following components:

- \ref DispatcherDocPage
- \ref LowLevelClientLibraryDocPage
- \ref CommonAPIDocPage

\tableofcontents


\include INSTALL.md


TODO
----

- Validate protocol implementation against Some/IP specification
- Add security features
- Implement IPv6 support
- Possibility to configure a local "subnet" in order to include only selected hosts in the service discovery process.
Implement more unit tests


Change log
----------

\include ChangeLog.md


FAQ
---
