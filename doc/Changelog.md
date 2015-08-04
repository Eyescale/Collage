# Changelog

# git master {#master}

* [160](https://github.com/Eyescale/Collage/pull/160): Replace command
  queue and barrier timeout exceptions with return values

# Release 1.2 (07-07-2015) {#Release12}

* [142](https://github.com/Eyescale/Collage/pull/142): Expose addConnection()
  for local server connections in Equalizer
* [143](https://github.com/Eyescale/Collage/pull/143),
  [144](https://github.com/Eyescale/Collage/pull/144),
  [152](https://github.com/Eyescale/Collage/pull/152): Denoise log output
* [147](https://github.com/Eyescale/Collage/pull/147): Adapt to
  Lunchbox/Pression refactoring

# Release 1.1 (07-08-2014) {#Release11}

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

# Known Bugs {#Bugs}

The following bugs were known at release time. Please file a [Bug Report]
(https://github.com/Eyescale/Collage/issues) if you find any other issue with
this release.

* [102](https://github.com/Eyescale/Collage/issues/102): test/connection fails
  for RDMA connections
* [57](https://github.com/Eyescale/Collage/issues/57): Windows: Larger number of
  concurrent receives causes intermittent blocking
* [15](https://github.com/Eyescale/Collage/issues/15): RDMAConnection
  not endian-safe
* [14](https://github.com/Eyescale/Collage/issues/14): coNetperf server
  occasionally crashes on client disconnect
* [2](https://github.com/Eyescale/Collage/issues/2): Multiple dispatcher
  inheritance not working with xlC
