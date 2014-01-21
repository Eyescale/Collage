
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "objectCM.h"

#include "nodeCommand.h"
#include "nullCM.h"
#include "node.h"
#include "object.h"
#include "objectDataICommand.h"
#include "objectInstanceDataOStream.h"
#include "objectDataOCommand.h"

co::ObjectCMPtr co::ObjectCM::ZERO = new co::NullCM;

#ifdef CO_INSTRUMENT_MULTICAST
lunchbox::a_int32_t co::ObjectCM::_hit( 0 );
lunchbox::a_int32_t co::ObjectCM::_miss( 0 );
#endif

namespace co
{
ObjectCM::ObjectCM( Object* object )
        : _object( object )
{}

ObjectCM::~ObjectCM()
{}

void ObjectCM::exit()
{
    lunchbox::ScopedFastWrite mutex( _lock );
    _object = 0;
}

void ObjectCM::push( const uint128_t& groupID, const uint128_t& typeID,
                     const Nodes& nodes )
{
    LBASSERT( _object );
    LBASSERT( !nodes.empty( ));
    if( nodes.empty( ))
    {
        LBWARN << "Push to an empty set of nodes" << std::endl;
        return;
    }

    ObjectInstanceDataOStream os( this );
    os.enablePush( getVersion(), nodes );
    _object->getInstanceData( os );

    // Send push notification to remote cmd thread while connections are valid
    OCommand( os.getConnections(), CMD_NODE_OBJECT_PUSH )
        << _object->getID() << groupID << typeID;

    os.disable(); // handled by remote recv thread
}

bool ObjectCM::sendSync( const MasterCMCommand& command )
{
    lunchbox::ScopedFastWrite mutex( _lock );
    if( !_object )
    {
        LBWARN << "Sync from detached object requested" << std::endl;
        return false;
    }

    const uint128_t& maxCachedVersion = command.getMaxCachedVersion();
    const bool useCache =
        command.useCache() &&
        command.getMasterInstanceID() == _object->getInstanceID() &&
        maxCachedVersion == getVersion();

    if( !useCache )
    {
        ObjectInstanceDataOStream os( this );
        os.enableSync( getVersion(), command );
        _object->getInstanceData( os );
        os.disable();
    }
    NodePtr node = command.getNode();
    node->send( CMD_NODE_SYNC_OBJECT_REPLY, useCache /*preferMulticast*/ )
        << node->getNodeID() << command.getObjectID() << command.getRequestID()
        << true << command.useCache() << useCache;
    return true;
}

bool ObjectCM::_addSlave( const MasterCMCommand& command,
                          const uint128_t& version )
{
    LBASSERT( version != VERSION_NONE );
    LBASSERT( command.getType() == COMMANDTYPE_NODE );
    LBASSERT( command.getCommand() == CMD_NODE_MAP_OBJECT );

    // process request
    if( command.getRequestedVersion() == VERSION_NONE )
    {
        // no data to send, send empty version
        _sendMapSuccess( command, false /* mc */ );
        _sendEmptyVersion( command, VERSION_NONE, false /* mc */ );
        _sendMapReply( command, VERSION_NONE, true, false, false /* mc */ );
        return true;
    }

    const bool replyUseCache = command.useCache() &&
                   (command.getMasterInstanceID() == _object->getInstanceID( ));
    return _initSlave( command, version, replyUseCache );
}

bool ObjectCM::_initSlave( const MasterCMCommand& command,
                           const uint128_t& replyVersion, bool replyUseCache )
{
#if 0
    LBLOG( LOG_OBJECTS ) << "Object id " << _object->_id << " v" << _version
                         << ", instantiate on " << node->getNodeID()
                         << std::endl;
#endif

#ifndef NDEBUG
    const uint128_t& version = command.getRequestedVersion();
    if( version != VERSION_OLDEST && version < replyVersion )
        LBINFO << "Mapping version " << replyVersion << " instead of "
               << version << std::endl;
#endif

    if( replyUseCache &&
        command.getMinCachedVersion() <= replyVersion &&
        command.getMaxCachedVersion() >= replyVersion )
    {
#ifdef CO_INSTRUMENT_MULTICAST
        ++_hit;
#endif
        _sendMapSuccess( command, false );
        _sendMapReply( command, replyVersion, true, replyUseCache, false );
        return true;
    }

    lunchbox::ScopedFastWrite mutex( _lock );
    if( !_object )
    {
        LBWARN << "Map to detached object requested" << std::endl;
        return false;
    }

#ifdef CO_INSTRUMENT_MULTICAST
    ++_miss;
#endif
    replyUseCache = false;

    _sendMapSuccess( command, true );

    // send instance data
    ObjectInstanceDataOStream os( this );

    os.enableMap( replyVersion, command.getNode(), command.getInstanceID( ));
    _object->getInstanceData( os );
    os.disable();
    if( !os.hasSentData( ))
        // no data, send empty command to set version
        _sendEmptyVersion( command, replyVersion, true /* mc */ );

    _sendMapReply( command, replyVersion, true, replyUseCache, true );
    return true;
}

void ObjectCM::_sendMapSuccess( const MasterCMCommand& command,
                                const bool multicast )
{
    command.getNode()->send( CMD_NODE_MAP_OBJECT_SUCCESS, multicast )
            << command.getNode()->getNodeID() << command.getObjectID()
            << command.getRequestID() << command.getInstanceID()
            << _object->getChangeType() << _object->getInstanceID();
}

void ObjectCM::_sendMapReply( const MasterCMCommand& command,
                              const uint128_t& version, const bool result,
                              const bool useCache, const bool multicast )
{
    command.getNode()->send( CMD_NODE_MAP_OBJECT_REPLY, multicast )
            << command.getNode()->getNodeID() << command.getObjectID()
            << version << command.getRequestID() << result
            << command.useCache() << useCache;
}

void ObjectCM::_sendEmptyVersion( const MasterCMCommand& command,
                                  const uint128_t& version,
                                  const bool multicast )
{
    NodePtr node = command.getNode();
    ConnectionPtr connection = node->getConnection( multicast );

    ObjectDataOCommand( Connections( 1, connection ), CMD_OBJECT_INSTANCE,
                        COMMANDTYPE_OBJECT, _object->getID(),
                        command.getInstanceID(), version, 0, 0, true, 0 )
            << NodeID() << _object->getInstanceID();
}

}
