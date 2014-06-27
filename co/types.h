
/* Copyright (c) 2006-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *               2011-2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *
 * This file is part of Collage <https://github.com/Eyescale/Collage>
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

#define CO_INSTANCE_MAX     LB_MAX_UINT32 //!< The biggest instance id possible
#define CO_INSTANCE_NONE    0xfffffffdu   //!< None/NULL identifier
#define CO_INSTANCE_INVALID 0xfffffffeu   //!< Invalid/unset instance identifier
#define CO_INSTANCE_ALL     0xffffffffu   //!< all object instances

class Barrier;
class Buffer;
class CommandQueue;
class Connection;
class ConnectionDescription;
class ConnectionListener;
class CustomICommand;
class CustomOCommand;
class DataIStream;
class DataOStream;
class Global;
class ICommand;
class LocalNode;
class Node;
class OCommand;
class Object;
class ObjectICommand;
class ObjectDataICommand;
class ObjectDataIStream;
class ObjectDataOCommand;
class ObjectFactory;
class ObjectHandler;
class ObjectOCommand;
class QueueItem;
class QueueMaster;
class QueueSlave;
class Serializable;
class Zeroconf;
template< class Q > class WorkerThread;
struct ObjectVersion;


using lunchbox::Array;
using lunchbox::Strings;
using lunchbox::StringsCIter;
using lunchbox::a_ssize_t;
using lunchbox::f_bool_t;
using lunchbox::uint128_t;
#ifdef LUNCHBOX_USE_V1_API
using lunchbox::UUID;
#endif

typedef uint128_t NodeID; //!< A unique identifier for nodes.

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

/** A vector of NodeID's. */
typedef std::vector< NodeID >                    NodeIDs;

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

/** A vector of input commands. */
typedef std::vector< ICommand >                    ICommands;
/** A iterator for a vector of input commands. */
typedef ICommands::iterator                        ICommandsIter;
/** A const iterator for a vector of input commands. */
typedef ICommands::const_iterator                  ICommandsCIter;

/** @cond IGNORE */
class BufferListener;
class MasterCMCommand;
class SendToken;

typedef lunchbox::RefPtr< Buffer > BufferPtr;
typedef lunchbox::RefPtr< const Buffer > ConstBufferPtr;

typedef std::vector< ObjectVersion > ObjectVersions;
typedef ObjectVersions::const_iterator ObjectVersionsCIter;
typedef std::deque< ObjectDataIStream* > ObjectDataIStreamDeque;
typedef std::vector< ObjectDataIStream* > ObjectDataIStreams;
/** @endcond */

}
#endif // CO_TYPES_H
