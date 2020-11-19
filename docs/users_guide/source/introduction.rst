Introduction
============

The SNIC Storage Explorer is a graphical client for access grid storage resources. The client builds on the ARC middlware to support most grid storage protocols. The basic design of the application is a multi-window file browser. Copying between resources are accomplished by drag and drop between file browser windows. To limit and optimise the bandwidth for transferring files, the application implements a file transfer list which can be configured with a maximum number of simultaneous transfers. Most operations in the application are also implemented using threads, to prevent locking up the user interface. However, since many file operations can take a long time to complete some user interface operations disable interaction while operations are ongoing. As the SNIC Storage application is a multi-window application, work on different storage resource can be continued by opening an additional storage window. 

SNIC Storage Explorer currently support the following protocols.

 * gsiftp - GridFTP
 * srm - Storage Resrouce Manager
 * https - WebDav with X509 authentication
 * local - Local file systems

 In future versions we are also planning for iRODS support.