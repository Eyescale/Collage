[TOC]

# Introduction {#Introduction}

Collage is a cross-platform C++ library for building heterogenous, distributed
applications. Among other things, it is the cluster backend for the [Equalizer
parallel rendering framework](https://github.com/Eyescale/Equalizer). Collage
provides an abstraction of different network connections, peer-to-peer
messaging, node discovery, synchronization and high-performance,
object-oriented, versioned data distribution. Collage is designed for
low-overhead multi-threaded execution which allows applications to easily
exploit multi-core architectures.

The
[API documentation](http://eyescale.github.io/Collage-1.2/index.html) can be
found on [eyescale.github.io](http://eyescale.github.io/). As with any open
source project, the available source code, in particular the
[unit tests](https://github.com/Eyescale/Collage/tree/1.2/tests) provide a
reference for developing applications.

Technical questions can be posted to the Equalizer
[Developer Mailing List](http://www.equalizergraphics.com/cgi-bin/mailman/listinfo/eq-dev),
or directly to
[info@equalizergraphics.com](mailto:info@equalizergraphics.com?subject=Collage%20question).

Commercial support, custom software development and porting services are
available from [Eyescale](http://www.eyescale.ch). Please contact
[info@eyescale.ch](mailto:info@eyescale.ch?subject=Collage%20support)
for further information.

# Features {#Features}

Collage provides different levels of abstraction to facilitate the
development distributed applications:

* Network Connections: stream-oriented point-to-point and reliable multicast
  connections for TCP/IP, SDP, InfiniBand RDMA, UDT, events, named and anonymous
  pipes, memory buffers and reliable multicast over UDP.
* Peer-to-Peer Messaging: Endian-safe node-to-node message communication with
  thread-aware message dispatch.
* Synchronization: Distributed barriers and synchronous messaging.
* Object data distribution: high-performance, object-oriented, versioned data
  distribution for C++ objects based on delta serialization.

# Building {#Building}

Collage is a cross-platform library, designed to run on any modern operating
system, including all Unix variants and the Windows operating system. Collage
uses CMake to create a platform-specific build environment. The following
platforms and build environments are tested:

* Linux: Ubuntu 14.04, RHEL 6.6 (Makefile, Ninja)
* Windows: 7 (Visual Studio 2012)
* Mac OS X: 10.8 (Makefile, Ninja)

~~~
git clone https://github.com/Eyescale/Collage.git
cd Collage
mkdir build
cd build
cmake ..
make
~~~
