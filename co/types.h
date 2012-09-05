
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *               2011-2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef CO_TYPES_H
#define CO_TYPES_H

#include <co/defines.h>
#include <lunchbox/refPtr.h>
#include <lunchbox/types.h>

#include <deque>
#include <vector>

namespace co
{

#define CO_SEPARATOR '#'

#define EQ_INSTANCE_MAX     LB_MAX_UINT32 //!< The biggest instance id possible
#define EQ_INSTANCE_NONE    0xfffffffdu   //!< None/NULL identifier
#define EQ_INSTANCE_INVALID 0xfffffffeu   //!< Invalid/unset instance identifier
#define EQ_INSTANCE_ALL     0xffffffffu   //!< all object instances

class Barrier;
class Buffer;
class CPUCompressor; //!< @internal
class Command;
class CommandQueue;
class Connection;
class ConnectionDescription;
class ConnectionListener;
class DataIStream;
class DataOStream;
class Global;
class LocalNode;
class Node;
class NodeOCommand;
class Object;
class ObjectFactory;
class ObjectHandler;
class ObjectDataCommand;
class ObjectDataIStream;
class ObjectDataOCommand;
class ObjectCommand;
class ObjectOCommand;
class Plugin;        //!< @internal
class PluginRegistry;
class QueueItem;
class QueueMaster;
class QueueSlave;
class Serializable;
class Zeroconf;
struct CompressorInfo; //!< @internal
template< class Q > class WorkerThread;
struct ObjectVersion;

using lunchbox::UUID;
using lunchbox::uint128_t;
using lunchbox::Strings;
using lunchbox::StringsCIter;

typedef UUID NodeID; //!< A unique identifier for nodes.

/** A reference pointer for Node pointers. */
typedef lunchbox::RefPtr< Node >                  NodePtr;
/** A reference pointer for const Node pointers. */
typedef lunchbox::RefPtr< const Node >            ConstNodePtr;
/** A reference pointer for LocalNode pointers. */
typedef lunchbox::RefPtr< LocalNode >             LocalNodePtr;
/** A reference pointer for const LocalNode pointers. */
typedef lunchbox::RefPtr< const LocalNode >       ConstLocalNodePtr;
/** A reference pointer for Connection pointers. */
typedef lunchbox::RefPtr< Connection >            ConnectionPtr;
/** A reference pointer for ConnectionDescription pointers. */
typedef lunchbox::RefPtr< ConnectionDescription > ConnectionDescriptionPtr;
/** A reference pointer for const ConnectionDescription pointers. */
typedef lunchbox::RefPtr< const ConnectionDescription >
                                                  ConstConnectionDescriptionPtr;
/** A vector of ConnectionListener */
typedef std::vector< ConnectionListener* > ConnectionListeners;

/** A vector of NodePtr's. */
typedef std::vector< NodePtr >                   Nodes;
/** An iterator for a vector of nodes. */
typedef Nodes::iterator                          NodesIter;
/** A const iterator for a vector of nodes. */
typedef Nodes::const_iterator                    NodesCIter;

/** A vector of objects. */
typedef std::vector< Object* >                   Objects;
/** A iterator for a vector of objects. */
typedef Objects::iterator                        ObjectsIter;
/** A const iterator for a vector of objects. */
typedef Objects::const_iterator                  ObjectsCIter;

typedef std::vector< Barrier* > Barriers; //!< A vector of barriers
typedef Barriers::iterator BarriersIter;  //!< Barriers iterator
typedef Barriers::const_iterator BarriersCIter; //!< Barriers const iterator

/** A vector of ConnectionPtr's. */
typedef std::vector< ConnectionPtr >             Connections;
/** A const iterator for a vector of ConnectionPtr's. */
typedef Connections::const_iterator ConnectionsCIter;
/** An iterator for a vector of ConnectionPtr's. */
typedef Connections::iterator   ConnectionsIter;

/** A vector of ConnectionDescriptionPtr's. */
typedef std::vector< ConnectionDescriptionPtr >  ConnectionDescriptions;
/** An iterator for a vector of ConnectionDescriptionPtr's. */
typedef ConnectionDescriptions::iterator         ConnectionDescriptionsIter;
/** A const iterator for a vector of ConnectionDescriptionPtr's. */
typedef ConnectionDescriptions::const_iterator   ConnectionDescriptionsCIter;

/** @cond IGNORE */
class MasterCMCommand;
typedef lunchbox::RefPtr< Buffer > BufferPtr;
typedef std::deque< Command > CommandDeque;

typedef std::vector< ObjectVersion > ObjectVersions;
typedef ObjectVersions::const_iterator ObjectVersionsCIter;
typedef std::deque< ObjectDataIStream* > ObjectDataIStreamDeque;
typedef std::vector< ObjectDataIStream* > ObjectDataIStreams;

typedef std::vector< CompressorInfo > CompressorInfos;
typedef std::vector< const CompressorInfo* > CompressorInfoPtrs;
typedef std::vector< Plugin* > Plugins;

typedef CompressorInfos::const_iterator CompressorInfosCIter;
typedef Plugins::const_iterator PluginsCIter;
/** @endcond */

#ifndef EQ_2_0_API
namespace base
{
using namespace lunchbox;
using co::PluginRegistry;
using co::Global;
}
#endif

}
#endif // CO_TYPES_H
