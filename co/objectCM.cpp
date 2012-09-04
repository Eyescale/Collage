
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
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
#include "objectDataCommand.h"
#include "objectInstanceDataOStream.h"
#include "objectDataOCommand.h"

co::ObjectCM* co::ObjectCM::ZERO = new co::NullCM;

#ifdef EQ_INSTRUMENT_MULTICAST
lunchbox::a_int32_t co::ObjectCM::_hit( 0 );
lunchbox::a_int32_t co::ObjectCM::_miss( 0 );
#endif

namespace co
{
ObjectCM::ObjectCM( Object* object )
        : _object( object )
{}

void ObjectCM::push( const uint128_t& groupID, const uint128_t& typeID,
                     const Nodes& nodes )
{
    LBASSERT( _object );
    LBASSERT( !nodes.empty( ));
    if( nodes.empty( ))
        return;

    ObjectInstanceDataOStream os( this );
    os.enablePush( getVersion(), nodes );
    _object->getInstanceData( os );

    NodeOCommand( os.getConnections(), CMD_NODE_OBJECT_PUSH )
            << _object->getID() << groupID << typeID;
    os.disable();
}

void ObjectCM::_addSlave( Command& cmd, const uint128_t& version )
{
    // #145 introduce reset() on command to read from the buffer front
    Command command( cmd );

    LBASSERT( version != VERSION_NONE );
    LBASSERT( command.getType() == COMMANDTYPE_CO_NODE );
    LBASSERT( command.getCommand() == CMD_NODE_MAP_OBJECT );

    NodePtr node = command.getNode();

    const uint128_t& requested = command.get< uint128_t >();
    /*const uint128_t& minCachedVersion = */command.get< uint128_t >();
    /*const uint128_t& maxCachedVersion = */command.get< uint128_t >();
    const UUID& id = command.get< UUID >();
    /*const uint64_t maxVersion = */command.get< uint64_t >();
    const uint32_t requestID = command.get< uint32_t >();
    const uint32_t instanceID = command.get< uint32_t >();
    const uint32_t masterInstanceID = command.get< uint32_t >();
    const bool useCache = command.get< bool >();

    // process request
    if( requested == VERSION_NONE ) // no data to send, send empty version
    {
        _sendMapSuccess( node, id, requestID, instanceID, false );
        _sendEmptyVersion( node, instanceID, version, false /* mc */ );
        _sendMapReply( node, id, requestID, version, true, useCache, false,
                       false );

        return;
    }

    const bool replyUseCache = useCache &&
                               (masterInstanceID == _object->getInstanceID( ));
    _initSlave( node, requested, command, version, replyUseCache );
}

void ObjectCM::_initSlave( NodePtr node, const uint128_t& version,
                           Command& cmd, const uint128_t& replyVersion,
                           bool replyUseCache )
{
#if 0
    LBLOG( LOG_OBJECTS ) << "Object id " << _object->_id << " v" << _version
                         << ", instantiate on " << node->getNodeID()
                         << std::endl;
#endif

#ifndef NDEBUG
    if( version != VERSION_OLDEST && version < replyVersion )
        LBINFO << "Mapping version " << replyVersion << " instead of "
               << version << std::endl;
#endif

    // #145 introduce reset() on command to read from the buffer front
    Command command( cmd );

    /*const uint128_t& requested = */command.get< uint128_t >();
    const uint128_t& minCachedVersion = command.get< uint128_t >();
    const uint128_t& maxCachedVersion = command.get< uint128_t >();
    const UUID& id = command.get< UUID >();
    /*const uint64_t maxVersion = */command.get< uint64_t >();
    const uint32_t requestID = command.get< uint32_t >();
    const uint32_t instanceID = command.get< uint32_t >();
    /*const uint32_t masterInstanceID = */command.get< uint32_t >();
    const bool useCache = command.get< bool >();

    if( replyUseCache &&
        minCachedVersion <= replyVersion &&
        maxCachedVersion >= replyVersion )
    {
#ifdef EQ_INSTRUMENT_MULTICAST
        ++_hit;
#endif
        _sendMapSuccess( node, id, requestID, instanceID, false );
        _sendMapReply( node, id, requestID, replyVersion, true, useCache,
                       replyUseCache, false );
        return;
    }

#ifdef EQ_INSTRUMENT_MULTICAST
    ++_miss;
#endif
    replyUseCache = false;

    _sendMapSuccess( node, id, requestID, instanceID, true );

    // send instance data
    ObjectInstanceDataOStream os( this );

    os.enableMap( replyVersion, node, instanceID );
    _object->getInstanceData( os );
    os.disable();
    if( !os.hasSentData( ))
        // no data, send empty packet to set version
        _sendEmptyVersion( node, instanceID, replyVersion, true /* mc */ );

    _sendMapReply( node, id, requestID, replyVersion, true, useCache,
                   replyUseCache, true );
}

void ObjectCM::_sendMapSuccess( NodePtr node, const UUID& objectID,
                                const uint32_t requestID,
                                const uint32_t instanceID, const bool multicast)
{
    node->send( CMD_NODE_MAP_OBJECT_SUCCESS, COMMANDTYPE_CO_NODE, multicast )
            << node->getNodeID() << objectID << requestID << instanceID
            << _object->getChangeType() << _object->getInstanceID();
}

void ObjectCM::_sendMapReply( NodePtr node, const UUID& objectID,
                              const uint32_t requestID,
                              const uint128_t& version, const bool result,
                              const bool releaseCache, const bool useCache,
                              const bool multicast )
{
    node->send( CMD_NODE_MAP_OBJECT_REPLY, COMMANDTYPE_CO_NODE, multicast )
            << node->getNodeID() << objectID << version << requestID << result
            << releaseCache << useCache;
}

void ObjectCM::_sendEmptyVersion( NodePtr node, const uint32_t instanceID,
                                  const uint128_t& version,
                                  const bool multicast )
{
    ConnectionPtr connection = multicast ? node->useMulticast() : 0;
    if( !connection )
        connection = node->getConnection();

    ObjectDataOCommand( Connections( 1, connection ), CMD_OBJECT_INSTANCE,
                        COMMANDTYPE_CO_OBJECT, _object->getID(), instanceID,
                        version, 0, 0, true, 0 )
            << NodeID::ZERO << _object->getInstanceID();
}

}
