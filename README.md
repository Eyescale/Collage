# Collage

Collage is a cross-platform C++ library for building heterogenous, distributed
applications. Among other things, it is the cluster backend for the [Equalizer
parallel rendering framework](https://github.com/Eyescale/Equalizer). Collage
provides an abstraction of different network connections, peer-to-peer
messaging, node discovery, synchronization and high-performance,
object-oriented, versioned data distribution. Collage is designed for
low-overhead multi-threaded execution which allows applications to easily
exploit multi-core architectures.

## Features

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

## Downloads

* [Ubuntu Packages Repository](https://launchpad.net/~eilemann/+archive/equalizer/)
* [API Documentation](http://eyescale.github.com/)
* Building from source:

```
  git clone https://github.com/Eyescale/Buildyard.git
  cd Buildyard
  git clone https://github.com/Eyescale/config.git config.eyescale
  make Collage
```

### Version 1.0

* [Source Code](https://github.com/Eyescale/Collage/archive/1.0.1.tar.gz)
