Release Notes {#mainpage}
============

[TOC]

# Introduction {#Introduction}

Welcome to Collage, a high-performance C++ library for developing
object-oriented distributed applications. This release introduces the
official 1.0 API of the former Equalizer network library.

Collage 1.0 is a stable release of over eight years of development and
decades of experience into a high-performance and mature C++ library. It
is intended for all C++ developers creating distributed applications
with high-level abstractions. Collage 1.0 can be retrieved by
downloading the source code or one of the precompiled packages.

## Features {#Features}

Collage provides different levels of abstraction to facilitate the
development distributed applications:

* Network Connections: stream-oriented point-to-point and reliable
  multicast connections for TCP/IP, SDP, InfiniBand RDMA, UDT, events,
  named and anonymous pipes, memory buffers and reliable multicast over
  UDP.
* Peer-to-Peer Messaging: Endian-safe node-to-node message communication
  with thread-aware message dispatch.
* Synchronization: Distributed barriers and synchronous messaging.
* Object data distribution: high-performance, object-oriented, versioned
  data distribution for C++ objects based on delta serialization.

# New in this release {#New}

Collage 1.0 contains the following features, enhancements, bug fixes and
documentation changes over the Equalizer 1.4 release:

## New Features {#NewFeatures}

* Endian-safe messaging
* RDMA connection supported on Windows

## Enhancements {#Enhancements}

* Improved co::ObjectMap API and implementation

## Optimizations {#Optimizations}

* co::WorkerThread uses bulk message retrieval from co::CommandQueue

## Tools {#Tools}

* New coNodePerf application to benchmark node-to-node messaging performance

## Documentation {#Documentation}

The following documentation has been added or substantially improved
since the last release:

* Full documentation for the public Collage API
* Expanded Collage content in the Equalizer Programming and User Guide

## Bug Fixes {#Fixes}

Collage 1.0 includes various bugfixes over the Equalizer 1.4 release,
including the following:

* [22](https://github.com/Eyescale/Collage/issues/22): Crash during
  concurrent object deregister and map
* [13](https://github.com/Eyescale/Collage/issues/13): Global argument
  parsing broken
* [3](https://github.com/Eyescale/Collage/issues/3): Snappy compressor
  does not work on PPC

## Known Bugs {#Bugs}

The following bugs were known at release time. Please file a [Bug Report](https://github.com/Eyescale/Collage/issues) if you find any other issue with this release.

* [16](https://github.com/Eyescale/Collage/issues/16): RSP Interface not
  bound on Linux
* [15](https://github.com/Eyescale/Collage/issues/15): RDMAConnection
  not endian-safe
* [14](https://github.com/Eyescale/Collage/issues/14): coNetperf server
  occasionally crashes on client disconnect
* [2](https://github.com/Eyescale/Collage/issues/2): Multiple dispatcher
  inheritance not working with xlC

# About {#About}

Collage is a cross-platform library, designed to run on any modern
operating system, including all Unix variants and the Windows operating
system. Collage uses CMake and
[Buildyard](https://github.com/Eyescale/Buildyard) to create a
platform-specific build environment. The following platforms and build
environments are tested for version 1.0:

* Linux: Ubuntu 12.04, 12.10, 13.04, RHEL 6.3 (Makefile, i386, x64)
* Windows: 7 (Visual Studio 2008, i386, x64)
* Mac OS X: 10.8 (Makefile, XCode, i386, x64)

The Equalizer Programming and User Guide covers the basics of Collage
programming. The API documentation can be found on
[github](http://eyescale.github.io/).

As with any open source project, the available source code, in
particular the shipped tools provide a reference for developing or
porting applications.

Technical questions can be posted to the eq-dev Mailing List, or
directly to info@equalizergraphics.com.

Commercial support, custom software development and porting services are
available from [Eyescale](http://www.eyescale.ch). Please contact
[info@eyescale.ch](mailto:info@eyescale.ch?subject=Collage%20support)
for further information.

# Errata
