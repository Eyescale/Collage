# Changelog {#Changelog}

# git master

* [189](https://github.com/Eyescale/Collage/pull/189):
  Add remote node launch API and implementation

# Release 1.4 (11-Mar-2016)

* [172](https://github.com/Eyescale/Collage/pull/172):
  Add support to distribute servus::Serializable (ZeroBuf) objects

# Release 1.3 (3-Nov-2015)

* [164](https://github.com/Eyescale/Collage/pull/164): Implement
  OS-chosen port number for RSP
* [160](https://github.com/Eyescale/Collage/pull/160): Replace command
  queue and barrier timeout exceptions with return values

# Release 1.2 (7-Jul-2015)

* [142](https://github.com/Eyescale/Collage/pull/142): Expose addConnection()
  for local server connections in Equalizer
* [143](https://github.com/Eyescale/Collage/pull/143),
  [144](https://github.com/Eyescale/Collage/pull/144),
  [152](https://github.com/Eyescale/Collage/pull/152): Denoise log output
* [147](https://github.com/Eyescale/Collage/pull/147): Adapt to
  Lunchbox/Pression refactoring

# Release 1.1 (7-Aug-2014)

* [69](https://github.com/Eyescale/Collage/pull/69): Refactor Barrier API for
  robustness, deprecate old API
* [71](https://github.com/Eyescale/Collage/issues/71): BufferCache race may lead
  to segmentation fault
* [79](https://github.com/Eyescale/Collage/issues/79): Node::connect race
  condition
* [82](https://github.com/Eyescale/Collage/issues/82): Barrier races and
  deadlocks with sync()
* [88](https://github.com/Eyescale/Collage/issues/88): LocalNode::handleData()
  asserts handling non-pending receive
* [112](https://github.com/Eyescale/Collage/pull/112): Fix racy connection
  handshake
* [113](https://github.com/Eyescale/Collage/pull/113): De-race
  EventConnection::close to fix Travis

# Release 1.0 (24-Jun-2013)

* 07/Mar/2013: PluginRegistry, Plugin and compressors are moved to
  Lunchbox.  co::Global still maintains the global Collage plugin
  registry.

* 11/Feb/2013: Node::useMulticast has been renamed to getMulticast. This
  method, and the previous getMulticast have been made protected.
  Node::getConnection has a flag to prefer a multicast connection.

* 28/Jan/2013: The program name and working directory have been moved
  from co::Global to eq::Global.

* 06/Sep/2012: New stream-based commands supersedes packet-based
  messaging. New send() methods in co::Node & co::Object replaces old
  API. All packets are superseded by NodeOCommand & ObjectOOCommand for
  sending commands, and by Command & ObjectCommand for receiving
  commands.

* 09/Aug/2012: Made co::DataOStream::write private. Use 'os <<
  co::Array< T >( ptr, num )' instead.

* 27/Jul/2012: Made co::DataIStream::read private. Use 'is >> co::Array<
  T >( ptr, num )' instead.
