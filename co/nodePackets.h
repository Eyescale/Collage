
/* Copyright (c) 2005-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder  <cedric.stalder@gmail.com>
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

#ifndef EQNET_NODEPACKETS_H
#define EQNET_NODEPACKETS_H

#include <co/packets.h> // base structs
#include <co/localNode.h> // used inline
#include <co/objectVersion.h> // VERSION_FOO values
#include <lunchbox/compiler.h> // align macros

#include "nodeCommand.h" // CMD enums

/** @cond IGNORE */
namespace co
{
    struct NodeMapObjectPacket : public NodePacket
    {
        NodeMapObjectPacket()
                : minCachedVersion( VERSION_HEAD )
                , maxCachedVersion( VERSION_NONE )
                , masterInstanceID( 0 )
                , useCache( false )
            {
                command = CMD_NODE_MAP_OBJECT;
                size    = sizeof( NodeMapObjectPacket );
            }

        uint128_t requestedVersion;
        uint128_t minCachedVersion;
        uint128_t maxCachedVersion;
        UUID objectID;
        uint64_t maxVersion;
        uint32_t requestID;
        uint32_t instanceID;
        uint32_t masterInstanceID;
        uint32_t useCache; // bool + valgrind padding
    };


    struct NodeMapObjectSuccessPacket : public NodePacket
    {
        NodeMapObjectSuccessPacket( 
            const NodeMapObjectPacket* request )
            {
                command    = CMD_NODE_MAP_OBJECT_SUCCESS;
                size       = sizeof( NodeMapObjectSuccessPacket ); 
                requestID  = request->requestID;
                objectID   = request->objectID;
                instanceID = request->instanceID;
            }
        
        NodeID nodeID;
        UUID objectID;
        uint32_t requestID;
        uint32_t instanceID;
        uint32_t changeType;
        uint32_t masterInstanceID;
    };

    struct NodeMapObjectReplyPacket : public NodePacket
    {
        NodeMapObjectReplyPacket( 
            const NodeMapObjectPacket* request )
                : objectID( request->objectID )
                , version( request->requestedVersion )
                , requestID( request->requestID )
                , pad1( 0 )
                , result( false )
                , releaseCache( request->useCache )
                , useCache( false )
            {
                command   = CMD_NODE_MAP_OBJECT_REPLY;
                size      = sizeof( NodeMapObjectReplyPacket ); 
                pad2[0]=pad2[1]=pad2[2]=pad2[3]=pad2[4]=0;
            }
        
        NodeID nodeID;
        const UUID objectID;
        uint128_t version;
        const uint32_t requestID;
        const uint32_t pad1;
        
        bool result;
        const bool releaseCache;
        bool useCache;
        bool pad2[5];
    };

    struct NodeRemoveNodePacket : public NodePacket
    {
        NodeRemoveNodePacket()
             : requestID( LB_UNDEFINED_UINT32 )
            {
                command = CMD_NODE_REMOVE_NODE;
                size    = sizeof( NodeRemoveNodePacket ); 
            }
        Node*        node;
        uint32_t     requestID;
    };

    struct NodeObjectPushPacket : public NodePacket
    {
        NodeObjectPushPacket( const uint128_t oID, const uint128_t gID,
                              const uint128_t tID )
                : objectID( oID ), groupID( gID ), typeID( tID )
            {
                command = CMD_NODE_OBJECT_PUSH;
                size    = sizeof( NodeObjectPushPacket );
            }

        const uint128_t objectID;
        const uint128_t groupID;
        const uint128_t typeID;
    };

    //------------------------------------------------------------
    inline std::ostream& operator << ( std::ostream& os, 
                                    const NodeMapObjectPacket* packet )
    {
        os << (NodePacket*)packet << " id " << packet->objectID << "." 
           << packet->instanceID << " req " << packet->requestID;
        return os;
    }
    inline std::ostream& operator << ( std::ostream& os, 
                             const NodeMapObjectSuccessPacket* packet )
    {
        os << (NodePacket*)packet << " id " << packet->objectID << "." 
           << packet->instanceID << " req " << packet->requestID;
        return os;
    }
    inline std::ostream& operator << ( std::ostream& os, 
                                       const NodeMapObjectReplyPacket* packet )
    {
        os << (NodePacket*)packet << " id " << packet->objectID << " req "
           << packet->requestID;
        return os;
    }


}
/** @endcond */

#endif // EQNET_NODEPACKETS_H
