
/* Copyright (c) 2005-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
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

#include "objectStore.h"

#include "connection.h"
#include "connectionDescription.h"
#include "global.h"
#include "instanceCache.h"
#include "log.h"
#include "masterCMCommand.h"
#include "nodeCommand.h"
#include "oCommand.h"
#include "objectCM.h"
#include "objectCommand.h"
#include "objectDataICommand.h"
#include "objectDataIStream.h"

#include <lunchbox/futureFunction.h>
#include <lunchbox/scopedMutex.h>

#include <boost/bind.hpp>

#include <limits>

//#define DEBUG_DISPATCH
#ifdef DEBUG_DISPATCH
#  include <set>
#endif

namespace co
{
typedef CommandFunc< ObjectStore > CmdFunc;
typedef lunchbox::FutureFunction< bool > FuturebImpl;

ObjectStore::ObjectStore( LocalNode* localNode, a_ssize_t* counters )
        : _localNode( localNode )
        , _instanceIDs( -0x7FFFFFFF )
        , _instanceCache( new InstanceCache( Global::getIAttribute(
                              Global::IATTR_INSTANCE_CACHE_SIZE ) * LB_1MB ) )
        , _counters( counters )
{
    LBASSERT( localNode );
    CommandQueue* queue = localNode->getCommandThreadQueue();

    localNode->_registerCommand( CMD_NODE_FIND_MASTER_NODE_ID,
        CmdFunc( this, &ObjectStore::_cmdFindMasterNodeID ), queue );
    localNode->_registerCommand( CMD_NODE_FIND_MASTER_NODE_ID_REPLY,
        CmdFunc( this, &ObjectStore::_cmdFindMasterNodeIDReply ), 0 );
    localNode->_registerCommand( CMD_NODE_ATTACH_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdAttach ), 0 );
    localNode->_registerCommand( CMD_NODE_DETACH_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdDetach ), 0 );
    localNode->_registerCommand( CMD_NODE_REGISTER_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdRegister ), queue );
    localNode->_registerCommand( CMD_NODE_DEREGISTER_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdDeregister ), queue );
    localNode->_registerCommand( CMD_NODE_MAP_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdMap ), queue );
    localNode->_registerCommand( CMD_NODE_MAP_OBJECT_SUCCESS,
        CmdFunc( this, &ObjectStore::_cmdMapSuccess ), 0 );
    localNode->_registerCommand( CMD_NODE_MAP_OBJECT_REPLY,
        CmdFunc( this, &ObjectStore::_cmdMapReply ), 0 );
    localNode->_registerCommand( CMD_NODE_UNMAP_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdUnmap ), 0 );
    localNode->_registerCommand( CMD_NODE_UNSUBSCRIBE_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdUnsubscribe ), queue );
    localNode->_registerCommand( CMD_NODE_OBJECT_INSTANCE,
        CmdFunc( this, &ObjectStore::_cmdInstance ), 0 );
    localNode->_registerCommand( CMD_NODE_OBJECT_INSTANCE_MAP,
        CmdFunc( this, &ObjectStore::_cmdInstance ), 0 );
    localNode->_registerCommand( CMD_NODE_OBJECT_INSTANCE_COMMIT,
        CmdFunc( this, &ObjectStore::_cmdInstance ), 0 );
    localNode->_registerCommand( CMD_NODE_OBJECT_INSTANCE_PUSH,
        CmdFunc( this, &ObjectStore::_cmdInstance ), 0 );
    localNode->_registerCommand( CMD_NODE_OBJECT_INSTANCE_SYNC,
        CmdFunc( this, &ObjectStore::_cmdInstance ), 0 );
    localNode->_registerCommand( CMD_NODE_DISABLE_SEND_ON_REGISTER,
        CmdFunc( this, &ObjectStore::_cmdDisableSendOnRegister ), queue );
    localNode->_registerCommand( CMD_NODE_REMOVE_NODE,
        CmdFunc( this, &ObjectStore::_cmdRemoveNode ), queue );
    localNode->_registerCommand( CMD_NODE_OBJECT_PUSH,
        CmdFunc( this, &ObjectStore::_cmdPush ), queue );
    localNode->_registerCommand( CMD_NODE_SYNC_OBJECT,
        CmdFunc( this, &ObjectStore::_cmdSync ), queue );
    localNode->_registerCommand( CMD_NODE_SYNC_OBJECT_REPLY,
        CmdFunc( this, &ObjectStore::_cmdSyncReply ), 0 );
}

ObjectStore::~ObjectStore()
{
    LBVERB << "Delete ObjectStore @" << (void*)this << std::endl;

#ifndef NDEBUG
    if( !_objects->empty( ))
    {
        LBWARN << _objects->size() << " attached objects in destructor"
               << std::endl;

        for( ObjectsHash::const_iterator i = _objects->begin();
             i != _objects->end(); ++i )
        {
            const Objects& objects = i->second;
            LBWARN << "  " << objects.size() << " objects with id "
                   << i->first << std::endl;

            for( Objects::const_iterator j = objects.begin();
                 j != objects.end(); ++j )
            {
                const Object* object = *j;
                LBINFO << "    object type " << lunchbox::className( object )
                       << std::endl;
            }
        }
    }
    //LBASSERT( _objects->empty( ))
#endif
   clear();
   delete _instanceCache;
   _instanceCache = 0;
}

void ObjectStore::clear( )
{
    LBASSERT( _objects->empty( ));
    expireInstanceData( 0 );
    LBASSERT( !_instanceCache || _instanceCache->isEmpty( ));

    _objects->clear();
    _sendQueue.clear();
}

void ObjectStore::disableInstanceCache()
{
    LBASSERT( _localNode->isClosed( ));
    delete _instanceCache;
    _instanceCache = 0;
}

void ObjectStore::expireInstanceData( const int64_t age )
{
    if( _instanceCache )
        _instanceCache->expire( age );
}

void ObjectStore::removeInstanceData( const NodeID& nodeID )
{
    if( _instanceCache )
        _instanceCache->remove( nodeID );
}

void ObjectStore::enableSendOnRegister()
{
    ++_sendOnRegister;
}

void ObjectStore::disableSendOnRegister()
{
    if( Global::getIAttribute( Global::IATTR_NODE_SEND_QUEUE_SIZE ) > 0 )
    {
        lunchbox::Request< void > request =
            _localNode->registerRequest< void >();
        _localNode->send( CMD_NODE_DISABLE_SEND_ON_REGISTER ) << request;
    }
    else // OPT
        --_sendOnRegister;
}

//---------------------------------------------------------------------------
// identifier master node mapping
//---------------------------------------------------------------------------
NodeID ObjectStore::findMasterNodeID( const uint128_t& identifier )
{
    LB_TS_NOT_THREAD( _commandThread );

    // OPT: look up locally first?
    Nodes nodes;
    _localNode->getNodes( nodes );

    // OPT: send to multiple nodes at once?
    for( NodesIter i = nodes.begin(); i != nodes.end(); ++i )
    {
        NodePtr node = *i;
        lunchbox::Request< NodeID > request =
            _localNode->registerRequest< NodeID >();

        LBLOG( LOG_OBJECTS ) << "Finding " << identifier << " on " << node
                             << " req " << request.getID() << std::endl;
        node->send( CMD_NODE_FIND_MASTER_NODE_ID ) << identifier << request;

        const NodeID& masterNodeID = request.wait( Global::getTimeout( ));

        if( masterNodeID != 0 )
        {
            LBLOG( LOG_OBJECTS ) << "Found " << identifier << " on "
                                 << masterNodeID << std::endl;
            return masterNodeID;
        }
    }

    return NodeID();
}

//---------------------------------------------------------------------------
// object mapping
//---------------------------------------------------------------------------
void ObjectStore::attach( Object* object, const uint128_t& id,
                          const uint32_t instanceID )
{
    LBASSERT( object );
    LB_TS_NOT_THREAD( _receiverThread );

    lunchbox::Request< void > request =
        _localNode->registerRequest< void >( object );
    _localNode->send( CMD_NODE_ATTACH_OBJECT ) << id << instanceID << request;
}

namespace
{
uint32_t _genNextID( lunchbox::a_int32_t& val )
{
    uint32_t result;
    do
    {
        const long id = ++val;
        result = static_cast< uint32_t >(
            static_cast< int64_t >( id ) + 0x7FFFFFFFu );
    }
    while( result > CO_INSTANCE_MAX );

    return result;
}
}

void ObjectStore::_attach( Object* object, const uint128_t& id,
                           const uint32_t inInstanceID )
{
    LBASSERT( object );
    LB_TS_THREAD( _receiverThread );

    uint32_t instanceID = inInstanceID;
    if( inInstanceID == CO_INSTANCE_INVALID )
        instanceID = _genNextID( _instanceIDs );

    object->attach( id, instanceID );

    {
        lunchbox::ScopedFastWrite mutex( _objects );
        Objects& objects = _objects.data[ id ];
        LBASSERTINFO( !object->isMaster() || objects.empty(),
                      "Attaching master " << *object << ", " <<
                      objects.size() << " attached objects with same ID, " <<
                      "first is " << ( objects[0]->isMaster() ? "master " :
                                       "slave " ) << *objects[0] );
        objects.push_back( object );
    }

    _localNode->flushCommands(); // redispatch pending commands

    LBLOG( LOG_OBJECTS ) << "attached " << *object << " @"
                         << static_cast< void* >( object ) << std::endl;
}

void ObjectStore::detach( Object* object )
{
    LBASSERT( object );
    LB_TS_NOT_THREAD( _receiverThread );

    lunchbox::Request< void > request = _localNode->registerRequest< void >();
    _localNode->send( CMD_NODE_DETACH_OBJECT )
        << object->getID() << object->getInstanceID() << request;
}

void ObjectStore::swap( Object* oldObject, Object* newObject )
{
    LBASSERT( newObject );
    LBASSERT( oldObject );
    LBASSERT( oldObject->isMaster() );
    LB_TS_THREAD( _receiverThread );

    if( !oldObject->isAttached() )
        return;

    LBLOG( LOG_OBJECTS ) << "Swap " << lunchbox::className( oldObject )
                         << std::endl;
    const uint128_t& id = oldObject->getID();

    lunchbox::ScopedFastWrite mutex( _objects );
    ObjectsHash::iterator i = _objects->find( id );
    LBASSERT( i != _objects->end( ));
    if( i == _objects->end( ))
        return;

    Objects& objects = i->second;
    Objects::iterator j = find( objects.begin(), objects.end(), oldObject );
    LBASSERT( j != objects.end( ));
    if( j == objects.end( ))
        return;

    newObject->transfer( oldObject );
    *j = newObject;
}

void ObjectStore::_detach( Object* object )
{
    // check also _cmdUnmapObject when modifying!
    LBASSERT( object );
    LB_TS_THREAD( _receiverThread );

    if( !object->isAttached() )
        return;

    const uint128_t& id = object->getID();

    LBASSERT( _objects->find( id ) != _objects->end( ));
    LBLOG( LOG_OBJECTS ) << "Detach " << *object << std::endl;

    Objects& objects = _objects.data[ id ];
    Objects::iterator i = find( objects.begin(),objects.end(), object );
    LBASSERT( i != objects.end( ));

    {
        lunchbox::ScopedFastWrite mutex( _objects );
        objects.erase( i );
        if( objects.empty( ))
            _objects->erase( id );
    }

    LBASSERT( object->getInstanceID() != CO_INSTANCE_INVALID );
    object->detach();
    return;
}

uint32_t ObjectStore::mapNB( Object* object, const uint128_t& id,
                             const uint128_t& version, NodePtr master )
{
    LB_TS_NOT_THREAD( _receiverThread );
    LBLOG( LOG_OBJECTS )
        << "Mapping " << lunchbox::className( object ) << " to id " << id
        << " version " << version << std::endl;
    LBASSERT( object );
    LBASSERTINFO( id.isUUID(), id );

    if( !master )
        master = _localNode->connectObjectMaster( id );

    if( !master || !master->isReachable( ))
    {
        LBWARN << "Mapping of object " << id << " failed, invalid master node"
               << std::endl;
        return LB_UNDEFINED_UINT32;
    }

    if( !object || !id.isUUID( ))
    {
        LBWARN << "Invalid object " << object << " or id " << id << std::endl;
        return LB_UNDEFINED_UINT32;
    }

    const bool isAttached = object->isAttached();
    const bool isMaster = object->isMaster();
    LBASSERT( !isAttached );
    LBASSERT( !isMaster ) ;
    if( isAttached || isMaster )
    {
        LBWARN << "Invalid object state: attached " << isAttached << " master "
               << isMaster << std::endl;
        return LB_UNDEFINED_UINT32;
    }

    lunchbox::Request< void > request =
        _localNode->registerRequest< void >( object );
    uint128_t minCachedVersion = VERSION_HEAD;
    uint128_t maxCachedVersion = VERSION_NONE;
    uint32_t masterInstanceID = 0;
    const bool useCache = _checkInstanceCache( id, minCachedVersion,
                                               maxCachedVersion,
                                               masterInstanceID );
    object->notifyAttach();
    master->send( CMD_NODE_MAP_OBJECT )
        << version << minCachedVersion << maxCachedVersion << id
        << object->getMaxVersions() << request << _genNextID( _instanceIDs )
        << masterInstanceID << useCache;
    request.relinquish();
    return request.getID();
}

bool ObjectStore::_checkInstanceCache( const uint128_t& id, uint128_t& from,
                                       uint128_t& to, uint32_t& instanceID )
{
    if( !_instanceCache )
        return false;

    const InstanceCache::Data& cached = (*_instanceCache)[ id ];
    if( cached == InstanceCache::Data::NONE )
        return false;

    const ObjectDataIStreamDeque& versions = cached.versions;
    LBASSERT( !cached.versions.empty( ));
    instanceID = cached.masterInstanceID;
    from = versions.front()->getVersion();
    to = versions.back()->getVersion();
    LBLOG( LOG_OBJECTS ) << "Object " << id << " have v" << from << ".." << to
                         << std::endl;
    return true;
}

bool ObjectStore::mapSync( const uint32_t requestID )
{
    if( requestID == LB_UNDEFINED_UINT32 )
        return false;

    void* data = _localNode->getRequestData( requestID );
    if( data == 0 )
        return false;

    Object* object = LBSAFECAST( Object*, data );
    uint128_t version = VERSION_NONE;
    _localNode->waitRequest( requestID, version );

    const bool mapped = object->isAttached();
    if( mapped )
        object->applyMapData( version ); // apply initial instance data

    object->notifyAttached();
    LBLOG( LOG_OBJECTS )
        << "Mapped " << lunchbox::className( object ) << std::endl;
    return mapped;
}


f_bool_t ObjectStore::sync( Object* object, NodePtr master, const uint128_t& id,
                            const uint32_t instanceID )
{
    const uint32_t request = _startSync( object, master, id, instanceID );
    const FuturebImpl::Func& func = boost::bind( &ObjectStore::_finishSync,
                                                 this, request, object );
    return f_bool_t( new FuturebImpl( func ));
}

uint32_t ObjectStore::_startSync( Object* object, NodePtr master,
                                  const uint128_t& id,
                                  const uint32_t instanceID )
{
    LB_TS_NOT_THREAD( _receiverThread );
    LBLOG( LOG_OBJECTS )
        << "Syncing " << lunchbox::className( object ) << " with id " << id
        << std::endl;
    LBASSERT( object );
    LBASSERTINFO( id.isUUID(), id );

    if( !object || !id.isUUID( ))
    {
        LBWARN << "Invalid object " << object << " or id " << id << std::endl;
        return LB_UNDEFINED_UINT32;
    }

    if( !master )
        master = _localNode->connectObjectMaster( id );

    if( !master || !master->isReachable( ))
    {
        LBWARN << "Mapping of object " << id << " failed, invalid master node"
               << std::endl;
        return LB_UNDEFINED_UINT32;
    }

    lunchbox::Request< void > request =
        _localNode->registerRequest< void >( new ObjectDataIStream );
    uint128_t minCachedVersion = VERSION_HEAD;
    uint128_t maxCachedVersion = VERSION_NONE;
    uint32_t cacheInstanceID = 0;

    bool useCache = _checkInstanceCache( id, minCachedVersion, maxCachedVersion,
                                         cacheInstanceID );
    if( useCache )
    {
        switch( instanceID )
        {
        case CO_INSTANCE_ALL:
            break;
        default:
            if( instanceID == cacheInstanceID )
                break;

            useCache = false;
            LBCHECK( _instanceCache->release( id, 1 ));
            break;
        }
    }

    // Use stream expected by MasterCMCommand
    master->send( CMD_NODE_SYNC_OBJECT )
        << VERSION_NEWEST << minCachedVersion << maxCachedVersion << id
        << uint64_t(0) /* maxVersions */ << request << instanceID
        << cacheInstanceID << useCache;
    request.relinquish();
    return request.getID();
}

bool ObjectStore::_finishSync( const uint32_t requestID, Object* object )
{
    if( requestID == LB_UNDEFINED_UINT32 )
        return false;

    void* data = _localNode->getRequestData( requestID );
    if( data == 0 )
        return false;

    ObjectDataIStream* is = LBSAFECAST( ObjectDataIStream*, data );

    bool ok = false;
    _localNode->waitRequest( requestID, ok );

    if( !ok )
    {
        LBWARN << "Object synchronization failed" << std::endl;
        delete is;
        return false;
    }

    is->waitReady();
    object->applyInstanceData( *is );
    LBLOG( LOG_OBJECTS ) << "Synced " << lunchbox::className( object )
                         << std::endl;
    delete is;
    return true;
}

void ObjectStore::unmap( Object* object )
{
    LBASSERT( object );
    if( !object->isAttached( )) // not registered
        return;

    const uint128_t& id = object->getID();

    LBLOG( LOG_OBJECTS ) << "Unmap " << object << std::endl;

    object->notifyDetach();

    // send unsubscribe to master, master will send detach command.
    LBASSERT( !object->isMaster( ));
    LB_TS_NOT_THREAD( _commandThread );

    const uint32_t masterInstanceID = object->getMasterInstanceID();
    if( masterInstanceID != CO_INSTANCE_INVALID )
    {
        NodePtr master = object->getMasterNode();
        LBASSERT( master )

        if( master && master->isReachable( ))
        {
            lunchbox::Request< void > request =
                _localNode->registerRequest< void >();
            master->send( CMD_NODE_UNSUBSCRIBE_OBJECT )
                << id << request << masterInstanceID << object->getInstanceID();
            request.wait();
            object->notifyDetached();
            return;
        }
        LBERROR << "Master node for object id " << id << " not connected"
                << std::endl;
    }

    // no unsubscribe sent: Detach directly
    detach( object );
    object->setupChangeManager( Object::NONE, false, 0, CO_INSTANCE_INVALID );
    object->notifyDetached();
}

bool ObjectStore::register_( Object* object )
{
    LBASSERT( object );
    LBASSERT( !object->isAttached( ));

    const uint128_t& id = object->getID( );
    LBASSERTINFO( id.isUUID(), id );

    object->notifyAttach();
    object->setupChangeManager( object->getChangeType(), true, _localNode,
                                CO_INSTANCE_INVALID );
    attach( object, id, CO_INSTANCE_INVALID );

    if( Global::getIAttribute( Global::IATTR_NODE_SEND_QUEUE_SIZE ) > 0 )
        _localNode->send( CMD_NODE_REGISTER_OBJECT ) << object;

    object->notifyAttached();

    LBLOG( LOG_OBJECTS ) << "Registered " << object << std::endl;
    return true;
}

void ObjectStore::deregister( Object* object )
{
    LBASSERT( object );
    if( !object->isAttached( )) // not registered
        return;

    LBLOG( LOG_OBJECTS ) << "Deregister " << *object << std::endl;
    LBASSERT( object->isMaster( ));

    object->notifyDetach();

    if( Global::getIAttribute( Global::IATTR_NODE_SEND_QUEUE_SIZE ) > 0  )
    {
        // remove from send queue
        lunchbox::Request< void > request =
            _localNode->registerRequest< void >();
        _localNode->send( CMD_NODE_DEREGISTER_OBJECT ) << request;
    }

    const uint128_t id = object->getID();
    detach( object );
    object->setupChangeManager( Object::NONE, true, 0, CO_INSTANCE_INVALID );
    if( _instanceCache )
        _instanceCache->erase( id );
    object->notifyDetached();
}

bool ObjectStore::notifyCommandThreadIdle()
{
    LB_TS_THREAD( _commandThread );
    if( _sendQueue.empty( ))
        return false;

    LBASSERT( _sendOnRegister > 0 );
    SendQueueItem& item = _sendQueue.front();

    if( item.age > _localNode->getTime64( ))
    {
        Nodes nodes;
        _localNode->getNodes( nodes, false );
        if( nodes.empty( ))
        {
            lunchbox::Thread::yield();
            return !_sendQueue.empty();
        }

        item.object->sendInstanceData( nodes );
    }
    _sendQueue.pop_front();
    return !_sendQueue.empty();
}

void ObjectStore::removeNode( NodePtr node )
{
    lunchbox::Request< void > request = _localNode->registerRequest< void >();
    _localNode->send( CMD_NODE_REMOVE_NODE ) << node.get() << request;
}

//===========================================================================
// ICommand handling
//===========================================================================
bool ObjectStore::dispatchObjectCommand( ICommand& cmd )
{
    LB_TS_THREAD( _receiverThread );
    ObjectICommand command( cmd );
    const uint128_t& id = command.getObjectID();
    const uint32_t instanceID = command.getInstanceID();

    ObjectsHash::const_iterator i = _objects->find( id );

    if( i == _objects->end( ))
        // When the instance ID is set to none, we only care about the command
        // when we have an object of the given ID (multicast)
        return ( instanceID == CO_INSTANCE_NONE );

    const Objects& objects = i->second;
    LBASSERTINFO( !objects.empty(), command );

    if( instanceID <= CO_INSTANCE_MAX )
    {
        for( Objects::const_iterator j = objects.begin(); j!=objects.end(); ++j)
        {
            Object* object = *j;
            if( instanceID == object->getInstanceID( ))
            {
                LBCHECK( object->dispatchCommand( command ));
                return true;
            }
        }
        LBERROR << "Can't find object instance " << instanceID << " for "
                << command << std::endl;
        LBUNREACHABLE;
        return false;
    }

    Objects::const_iterator j = objects.begin();
    Object* object = *j;
    LBCHECK( object->dispatchCommand( command ));

    for( ++j; j != objects.end(); ++j )
    {
        object = *j;
        LBCHECK( object->dispatchCommand( command ));
    }
    return true;
}

bool ObjectStore::_cmdFindMasterNodeID( ICommand& command )
{
    LB_TS_THREAD( _commandThread );

    const uint128_t& id = command.get< uint128_t >();
    const uint32_t requestID = command.get< uint32_t >();
    LBASSERT( id.isUUID( ));

    NodeID masterNodeID;
    {
        lunchbox::ScopedFastRead mutex( _objects );
        ObjectsHashCIter i = _objects->find( id );

        if( i != _objects->end( ))
        {
            const Objects& objects = i->second;
            LBASSERT( !objects.empty( ));

            for( ObjectsCIter j = objects.begin(); j != objects.end(); ++j )
            {
                Object* object = *j;
                if( object->isMaster( ))
                    masterNodeID = _localNode->getNodeID();
                else
                {
                    NodePtr master = object->getMasterNode();
                    if( master.isValid( ))
                        masterNodeID = master->getNodeID();
                }
                if( masterNodeID != 0 )
                    break;
            }
        }
    }

    LBLOG( LOG_OBJECTS ) << "Object " << id << " master " << masterNodeID
                         << " req " << requestID << std::endl;
    command.getNode()->send( CMD_NODE_FIND_MASTER_NODE_ID_REPLY )
            << masterNodeID << requestID;
    return true;
}

bool ObjectStore::_cmdFindMasterNodeIDReply( ICommand& command )
{
    const NodeID& masterNodeID = command.get< NodeID >();
    const uint32_t requestID = command.get< uint32_t >();
    _localNode->serveRequest( requestID, masterNodeID );
    return true;
}

bool ObjectStore::_cmdAttach( ICommand& command )
{
    LB_TS_THREAD( _receiverThread );
    LBLOG( LOG_OBJECTS ) << "Cmd attach object " << command << std::endl;

    const uint128_t& objectID = command.get< uint128_t >();
    const uint32_t instanceID = command.get< uint32_t >();
    const uint32_t requestID = command.get< uint32_t >();

    Object* object = static_cast< Object* >( _localNode->getRequestData(
                                                 requestID ));
    _attach( object, objectID, instanceID );
    _localNode->serveRequest( requestID );
    return true;
}

bool ObjectStore::_cmdDetach( ICommand& command )
{
    LB_TS_THREAD( _receiverThread );
    LBLOG( LOG_OBJECTS ) << "Cmd detach object " << command << std::endl;

    const uint128_t& objectID = command.get< uint128_t >();
    const uint32_t instanceID = command.get< uint32_t >();
    const uint32_t requestID = command.get< uint32_t >();

    ObjectsHash::const_iterator i = _objects->find( objectID );
    if( i != _objects->end( ))
    {
        const Objects& objects = i->second;

        for( Objects::const_iterator j = objects.begin();
             j != objects.end(); ++j )
        {
            Object* object = *j;
            if( object->getInstanceID() == instanceID )
            {
                _detach( object );
                break;
            }
        }
    }

    LBASSERT( requestID != LB_UNDEFINED_UINT32 );
    _localNode->serveRequest( requestID );
    return true;
}

bool ObjectStore::_cmdRegister( ICommand& command )
{
    LB_TS_THREAD( _commandThread );
    if( _sendOnRegister <= 0 )
        return true;

    LBLOG( LOG_OBJECTS ) << "Cmd register object " << command << std::endl;

    Object* object = reinterpret_cast< Object* >( command.get< void* >( ));

    const int32_t age = Global::getIAttribute(
                            Global::IATTR_NODE_SEND_QUEUE_AGE );
    SendQueueItem item;
    item.age = age ? age + _localNode->getTime64() :
                     std::numeric_limits< int64_t >::max();
    item.object = object;
    _sendQueue.push_back( item );

    const uint32_t size = Global::getIAttribute(
                             Global::IATTR_NODE_SEND_QUEUE_SIZE );
    while( _sendQueue.size() > size )
        _sendQueue.pop_front();

    return true;
}

bool ObjectStore::_cmdDeregister( ICommand& command )
{
    LB_TS_THREAD( _commandThread );
    LBLOG( LOG_OBJECTS ) << "Cmd deregister object " << command << std::endl;

    const uint32_t requestID = command.get< uint32_t >();

    const void* object = _localNode->getRequestData( requestID );

    for( SendQueueIter i = _sendQueue.begin(); i != _sendQueue.end(); ++i )
    {
        if( i->object == object )
        {
            _sendQueue.erase( i );
            break;
        }
    }

    _localNode->serveRequest( requestID );
    return true;
}

bool ObjectStore::_cmdMap( ICommand& cmd )
{
    LB_TS_THREAD( _commandThread );

    MasterCMCommand command( cmd );
    const uint128_t& id = command.getObjectID();

    LBLOG( LOG_OBJECTS ) << "Cmd map object " << command << " id " << id << "."
                         << command.getInstanceID() << " req "
                         << command.getRequestID() << std::endl;

    ObjectCMPtr masterCM;
    {
        lunchbox::ScopedFastRead mutex( _objects );
        ObjectsHash::const_iterator i = _objects->find( id );
        if( i != _objects->end( ))
        {
            const Objects& objects = i->second;

            for( ObjectsCIter j = objects.begin(); j != objects.end(); ++j )
            {
                Object* object = *j;
                if( object->isMaster( ))
                {
                    masterCM = object->_getChangeManager();
                    break;
                }
            }
        }
    }

    if( !masterCM || !masterCM->addSlave( command ))
    {
        LBWARN << "Can't find master object to map " << id << std::endl;
        NodePtr node = command.getNode();
        node->send( CMD_NODE_MAP_OBJECT_REPLY )
            << node->getNodeID() << id << command.getRequestedVersion()
            << command.getRequestID() << false << command.useCache() << false;
    }

    ++_counters[ LocalNode::COUNTER_MAP_OBJECT_REMOTE ];
    return true;
}

bool ObjectStore::_cmdMapSuccess( ICommand& command )
{
    LB_TS_THREAD( _receiverThread );

    const uint128_t& nodeID = command.get< uint128_t >();
    const uint128_t& objectID = command.get< uint128_t >();
    const uint32_t requestID = command.get< uint32_t >();
    const uint32_t instanceID = command.get< uint32_t >();
    const Object::ChangeType changeType = command.get< Object::ChangeType >();
    const uint32_t masterInstanceID = command.get< uint32_t >();

    // Map success commands are potentially multicasted (see above)
    // verify that we are the intended receiver
    if( nodeID != _localNode->getNodeID( ))
        return true;

    LBLOG( LOG_OBJECTS ) << "Cmd map object success " << command
                         << " id " << objectID << "." << instanceID
                         << " req " << requestID << std::endl;

    // set up change manager and attach object to dispatch table
    Object* object = static_cast< Object* >(
        _localNode->getRequestData( requestID ));
    LBASSERT( object );
    LBASSERT( !object->isMaster( ));

    object->setupChangeManager( Object::ChangeType( changeType ), false,
                                _localNode, masterInstanceID );
    _attach( object, objectID, instanceID );
    return true;
}

bool ObjectStore::_cmdMapReply( ICommand& command )
{
    LB_TS_THREAD( _receiverThread );

    // Map reply commands are potentially multicasted (see above)
    // verify that we are the intended receiver
    if( command.get< uint128_t >() != _localNode->getNodeID( ))
        return true;

    const uint128_t& objectID = command.get< uint128_t >();
    const uint128_t& version = command.get< uint128_t >();
    const uint32_t requestID = command.get< uint32_t >();
    const bool result = command.get< bool >();
    const bool releaseCache = command.get< bool >();
    const bool useCache = command.get< bool >();

    LBLOG( LOG_OBJECTS ) << "Cmd map object reply " << command << " id "
                         << objectID << " req " << requestID << std::endl;

    LBASSERT( _localNode->getRequestData( requestID ));

    if( result )
    {
        Object* object = static_cast<Object*>(
            _localNode->getRequestData( requestID ));
        LBASSERT( object );
        LBASSERT( !object->isMaster( ));

        object->setMasterNode( command.getNode( ));

        if( useCache )
        {
            LBASSERT( releaseCache );
            LBASSERT( _instanceCache );

            const uint128_t& id = objectID;
            const InstanceCache::Data& cached = (*_instanceCache)[ id ];
            LBASSERT( cached != InstanceCache::Data::NONE );
            LBASSERT( !cached.versions.empty( ));

            object->addInstanceDatas( cached.versions, version );
            LBCHECK( _instanceCache->release( id, 2 ));
        }
        else if( releaseCache )
        {
            LBCHECK( _instanceCache->release( objectID, 1 ));
        }
    }
    else
    {
        if( releaseCache )
            _instanceCache->release( objectID, 1 );

        LBWARN << "Could not map object " << objectID << std::endl;
    }

    _localNode->serveRequest( requestID, version );
    return true;
}

bool ObjectStore::_cmdUnsubscribe( ICommand& command )
{
    LB_TS_THREAD( _commandThread );
    LBLOG( LOG_OBJECTS ) << "Cmd unsubscribe object  " << command << std::endl;

    const uint128_t& id = command.get< uint128_t >();
    const uint32_t requestID = command.get< uint32_t >();
    const uint32_t masterInstanceID = command.get< uint32_t >();
    const uint32_t slaveInstanceID = command.get< uint32_t >();

    NodePtr node = command.getNode();

    {
        lunchbox::ScopedFastWrite mutex( _objects );
        ObjectsHash::const_iterator i = _objects->find( id );
        if( i != _objects->end( ))
        {
            const Objects& objects = i->second;
            for( ObjectsCIter j = objects.begin(); j != objects.end(); ++j )
            {
                Object* object = *j;
                if( object->isMaster() &&
                    object->getInstanceID() == masterInstanceID )
                {
                    object->removeSlave( node, slaveInstanceID );
                    break;
                }
            }
        }
    }

    node->send( CMD_NODE_DETACH_OBJECT ) << id << slaveInstanceID << requestID;
    return true;
}

bool ObjectStore::_cmdUnmap( ICommand& command )
{
    LB_TS_THREAD( _receiverThread );
    LBLOG( LOG_OBJECTS ) << "Cmd unmap object " << command << std::endl;

    const uint128_t& objectID = command.get< uint128_t >();

    if( _instanceCache )
        _instanceCache->erase( objectID );

    ObjectsHash::iterator i = _objects->find( objectID );
    if( i == _objects->end( )) // nothing to do
        return true;

    const Objects objects = i->second;
    {
        lunchbox::ScopedFastWrite mutex( _objects );
        _objects->erase( i );
    }

    for( Objects::const_iterator j = objects.begin(); j != objects.end(); ++j )
    {
        Object* object = *j;
        object->detach();
    }

    return true;
}

bool ObjectStore::_cmdSync( ICommand& cmd )
{
    LB_TS_THREAD( _commandThread );
    MasterCMCommand command( cmd );
    const uint128_t& id = command.getObjectID();
    LBINFO << command.getNode() << std::endl;

    LBLOG( LOG_OBJECTS ) << "Cmd sync object id " << id << "."
                         << command.getInstanceID() << " req "
                         << command.getRequestID() << std::endl;

    const uint32_t cacheInstanceID = command.getMasterInstanceID();
    ObjectCMPtr cm;
    {
        lunchbox::ScopedFastRead mutex( _objects );
        ObjectsHash::const_iterator i = _objects->find( id );
        if( i != _objects->end( ))
        {
            const Objects& objects = i->second;

            for( ObjectsCIter j = objects.begin(); j != objects.end(); ++j )
            {
                Object* object = *j;
                if( command.getInstanceID() == object->getInstanceID( ))
                {
                    cm = object->_getChangeManager();
                    LBASSERT( cm );
                    break;
                }

                if( command.getInstanceID() != CO_INSTANCE_ALL )
                    continue;

                cm = object->_getChangeManager();
                LBASSERT( cm );
                if( cacheInstanceID == object->getInstanceID( ))
                    break;
            }
            if( !cm )
                LBWARN << "Can't find object to sync " << id << "."
                       << command.getInstanceID() << " in " << objects.size()
                       << " instances" << std::endl;
        }
        if( !cm )
            LBWARN << "Can't find object to sync " << id
                   << ", no object with identifier" << std::endl;
    }
    if( !cm || !cm->sendSync( command ))
    {
        NodePtr node = command.getNode();
        node->send( CMD_NODE_SYNC_OBJECT_REPLY )
            << node->getNodeID() << id << command.getRequestID()
            << false << command.useCache() << false;
    }
    return true;
}

bool ObjectStore::_cmdSyncReply( ICommand& command )
{
    LB_TS_THREAD( _receiverThread );

    // Sync reply commands are potentially multicasted (see above)
    // verify that we are the intended receiver
    if( command.get< uint128_t >() != _localNode->getNodeID( ))
        return true;

    const NodeID& id = command.get< NodeID >();
    const uint32_t requestID = command.get< uint32_t >();
    const bool result = command.get< bool >();
    const bool releaseCache = command.get< bool >();
    const bool useCache = command.get< bool >();
    void* const data = _localNode->getRequestData( requestID );
    ObjectDataIStream* const is = LBSAFECAST( ObjectDataIStream*, data );

    LBLOG( LOG_OBJECTS ) << "Cmd sync object reply " << command << " req "
                         << requestID << std::endl;
    if( result )
    {
        if( useCache )
        {
            LBASSERT( releaseCache );
            LBASSERT( _instanceCache );

            const InstanceCache::Data& cached = (*_instanceCache)[ id ];
            LBASSERT( cached != InstanceCache::Data::NONE );
            LBASSERT( !cached.versions.empty( ));

            *is = *cached.versions.back();
            LBCHECK( _instanceCache->release( id, 2 ));
        }
        else if( releaseCache )
        {
            LBCHECK( _instanceCache->release( id, 1 ));
        }
    }
    else
    {
        if( releaseCache )
            _instanceCache->release( id, 1 );

        LBWARN << "Could not sync object " << id << " request " << requestID
               << std::endl;
    }

    _localNode->serveRequest( requestID, result );
    return true;
}

bool ObjectStore::_cmdInstance( ICommand& inCommand )
{
    LB_TS_THREAD( _receiverThread );
    LBASSERT( _localNode );

    ObjectDataICommand command( inCommand );
    const NodeID& nodeID = command.get< NodeID >();
    const uint32_t masterInstanceID = command.get< uint32_t >();
    const uint32_t cmd = command.getCommand();

    LBLOG( LOG_OBJECTS ) << "Cmd instance " << command << " master "
                         << masterInstanceID << " node " << nodeID << std::endl;

    command.setType( COMMANDTYPE_OBJECT );
    command.setCommand( CMD_OBJECT_INSTANCE );

    const uint128_t& version = command.getVersion();
    if( _instanceCache && version.high() == 0 )
    {
        const ObjectVersion rev( command.getObjectID(), version );
#ifndef CO_AGGRESSIVE_CACHING // Issue Equalizer#82:
        if( cmd != CMD_NODE_OBJECT_INSTANCE_PUSH )
#endif
            _instanceCache->add( rev, masterInstanceID, command, 0 );
    }

    switch( cmd )
    {
      case CMD_NODE_OBJECT_INSTANCE:
        LBASSERT( nodeID == 0 );
        LBASSERT( command.getInstanceID() == CO_INSTANCE_NONE );
        return true;

      case CMD_NODE_OBJECT_INSTANCE_MAP:
        if( nodeID != _localNode->getNodeID( )) // not for me
            return true;

        LBASSERT( command.getInstanceID() <= CO_INSTANCE_MAX );
        return dispatchObjectCommand( command );

      case CMD_NODE_OBJECT_INSTANCE_COMMIT:
        LBASSERT( nodeID == 0 );
        LBASSERT( command.getInstanceID() == CO_INSTANCE_NONE );
        return dispatchObjectCommand( command );

      case CMD_NODE_OBJECT_INSTANCE_PUSH:
        LBASSERT( nodeID == 0 );
        LBASSERT( command.getInstanceID() == CO_INSTANCE_NONE );
        _pushData.addDataCommand( command.getObjectID(), command );
        return true;

      case CMD_NODE_OBJECT_INSTANCE_SYNC:
      {
        if( nodeID != _localNode->getNodeID( )) // not for me
            return true;

        void* data = _localNode->getRequestData( command.getInstanceID( ));
        LBASSERT( command.getInstanceID() != CO_INSTANCE_NONE );
        LBASSERTINFO( data, this );

        ObjectDataIStream* is = LBSAFECAST( ObjectDataIStream*, data );
        is->addDataCommand( command );
        return true;
      }

      default:
        LBUNREACHABLE;
        return false;
    }
}

bool ObjectStore::_cmdDisableSendOnRegister( ICommand& command )
{
    LB_TS_THREAD( _commandThread );
    LBASSERTINFO( _sendOnRegister > 0, _sendOnRegister );

    if( --_sendOnRegister == 0 )
    {
        _sendQueue.clear();

        Nodes nodes;
        _localNode->getNodes( nodes, false );
        for( NodesCIter i = nodes.begin(); i != nodes.end(); ++i )
        {
            NodePtr node = *i;
            ConnectionPtr multicast = node->getConnection( true );
            ConnectionPtr connection = node->getConnection( false );
            if( multicast )
                multicast->finish();
            if( connection && connection != multicast )
                connection->finish();
        }
    }

    const uint32_t requestID = command.get< uint32_t >();
    _localNode->serveRequest( requestID );
    return true;
}

bool ObjectStore::_cmdRemoveNode( ICommand& command )
{
    LB_TS_THREAD( _commandThread );
    LBLOG( LOG_OBJECTS ) << "Cmd object  " << command << std::endl;

    Node* node = command.get< Node* >();
    const uint32_t requestID = command.get< uint32_t >();

    lunchbox::ScopedFastWrite mutex( _objects );
    for( ObjectsHashCIter i = _objects->begin(); i != _objects->end(); ++i )
    {
        const Objects& objects = i->second;
        for( ObjectsCIter j = objects.begin(); j != objects.end(); ++j )
            (*j)->removeSlaves( node );
    }

    if( requestID != LB_UNDEFINED_UINT32 )
        _localNode->serveRequest( requestID );
    else
        node->unref(); // node was ref'd before LocalNode::_handleDisconnect()

    return true;
}

bool ObjectStore::_cmdPush( ICommand& command )
{
    LB_TS_THREAD( _commandThread );

    const uint128_t& objectID = command.get< uint128_t >();
    const uint128_t& groupID = command.get< uint128_t >();
    const uint128_t& typeID = command.get< uint128_t >();

    ObjectDataIStream* is = _pushData.pull( objectID );

    _localNode->objectPush( groupID, typeID, objectID, *is );
    _pushData.recycle( is );
    return true;
}

std::ostream& operator << ( std::ostream& os, ObjectStore* objectStore )
{
    if( !objectStore )
    {
        os << "NULL objectStore";
        return os;
    }

    os << "objectStore (" << (void*)objectStore << ")";

    return os;
}
}
