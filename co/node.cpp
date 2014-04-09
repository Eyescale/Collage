
/* Copyright (c) 2005-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "node.h"

#include "connectionDescription.h"
#include "customOCommand.h"
#include "nodeCommand.h"
#include "oCommand.h"

#include <lunchbox/scopedMutex.h>
#include <lunchbox/spinLock.h>

namespace co
{
namespace
{
/** The state of the node. */
enum State
{
    STATE_CLOSED,    //!< initial state
    STATE_CONNECTED, //!< proxy for a remote node, connected
    STATE_LISTENING, //!< local node, listening
    STATE_CLOSING    //!< listening, about to close
};

struct MCData
{
    ConnectionPtr connection;
    NodePtr       node;
};
typedef std::vector< MCData > MCDatas;
}

namespace detail
{
class Node
{
public:
    /** Globally unique node identifier. */
    NodeID id;

    const uint32_t type;

    /** The current state of this node. */
    State state;

    /** The connection to this node. */
    ConnectionPtr outgoing;

    /** The multicast connection to this node, can be 0. */
    lunchbox::Lockable< ConnectionPtr > outMulticast;

    /**
     * Yet unused multicast connections for this node.
     *
     * On the first multicast send usage, the connection is 'primed' by sending
     * our node identifier to the MC group, removed from this vector and set as
     * outMulticast.
     */
    MCDatas multicasts;

    /** The list of descriptions on how this node is reachable. */
    lunchbox::Lockable< ConnectionDescriptions, lunchbox::SpinLock >
        connectionDescriptions;

    /** Last time commands were received */
    int64_t lastReceive;

    /** Is a big endian host? */
    bool bigEndian;

    Node( const uint32_t type_ )
        : id( lunchbox::make_UUID( ))
        , type( type_ )
        , state( STATE_CLOSED )
        , lastReceive ( 0 )
#ifdef COLLAGE_BIGENDIAN
        , bigEndian( true )
#else
        , bigEndian( false )
#endif
        {}

    ~Node()
    {
        LBASSERT( !outgoing );
        connectionDescriptions->clear();
    }
};
}

Node::Node( const uint32_t type )
        : _impl( new detail::Node( type ))
{
    LBVERB << "New Node @" << (void*)this << " " << _impl->id << std::endl;
}

Node::~Node()
{
    LBVERB << "Delete Node @" << (void*)this << " " << _impl->id << std::endl;
    delete _impl;
}

bool Node::operator == ( const Node* node ) const
{
    LBASSERTINFO( _impl->id != node->_impl->id || this == node,
                  "Two node instances with the same ID found "
                  << (void*)this << " and " << (void*)node );

    return ( _impl == node->_impl );
}

ConnectionDescriptions Node::getConnectionDescriptions() const
{
    lunchbox::ScopedFastRead mutex( _impl->connectionDescriptions );
    return _impl->connectionDescriptions.data;
}

ConnectionPtr Node::getMulticast()
{
    if( !isReachable( ))
        return 0;

    ConnectionPtr connection = _impl->outMulticast.data;
    if( connection && !connection->isClosed( ))
        return connection;

    lunchbox::ScopedMutex<> mutex( _impl->outMulticast );
    if( _impl->multicasts.empty( ))
        return 0;

    MCData data = _impl->multicasts.back();
    _impl->multicasts.pop_back();
    NodePtr node = data.node;

    // prime multicast connections on peers
    LBINFO << "Announcing id " << node->getNodeID() << " to multicast group "
           << data.connection->getDescription() << std::endl;

#ifdef COLLAGE_BIGENDIAN
    uint32_t cmd = CMD_NODE_ID_BE;
    lunchbox::byteswap( cmd );
#else
    const uint32_t cmd = CMD_NODE_ID;
#endif
    OCommand( Connections( 1, data.connection ), cmd )
        << node->getNodeID() << getType() << node->serialize();

    _impl->outMulticast.data = data.connection;
    return data.connection;
}

void Node::addConnectionDescription( ConnectionDescriptionPtr cd )
{
    LBASSERTINFO( isClosed(), *this );
    if( !isClosed( ))
        return;
    _addConnectionDescription( cd );
}

void Node::_addConnectionDescription( ConnectionDescriptionPtr cd )
{
    lunchbox::ScopedFastWrite mutex( _impl->connectionDescriptions );
    _impl->connectionDescriptions->push_back( cd );
}

bool Node::removeConnectionDescription( ConnectionDescriptionPtr cd )
{
    LBASSERTINFO( isClosed(), *this );
    if( !isClosed( ))
        return false;
    return _removeConnectionDescription( cd );
}

bool Node::_removeConnectionDescription( ConnectionDescriptionPtr cd )
{
    lunchbox::ScopedFastWrite mutex( _impl->connectionDescriptions );

    // Don't use std::find, RefPtr::operator== compares pointers, not values.
    for( ConnectionDescriptionsIter i = _impl->connectionDescriptions->begin();
         i != _impl->connectionDescriptions->end(); ++i )
    {
        if( *cd != **i )
            continue;

        _impl->connectionDescriptions->erase( i );
        return true;
    }
    return false;
}

std::string Node::serialize() const
{
    std::ostringstream data;
    data << Version::getMajor() << CO_SEPARATOR << Version::getMinor()
         << CO_SEPARATOR << _impl->id << CO_SEPARATOR << _impl->bigEndian
         << CO_SEPARATOR;
    {
        lunchbox::ScopedFastRead mutex( _impl->connectionDescriptions );
        data << co::serialize( _impl->connectionDescriptions.data );
    }
    return data.str();
}

bool Node::deserialize( std::string& data )
{
    LBASSERT( _impl->state == STATE_CLOSED );

    // version check
    int32_t major = 0;
    size_t nextPos = data.find( CO_SEPARATOR );
    if( nextPos == std::string::npos || nextPos == 0 )
    {
        LBERROR << "Could not parse node major version data" << std::endl;
        return false;
    }

    std::istringstream is( data.substr( 0, nextPos ));
    data = data.substr( nextPos + 1 );
    is >> major;

    int32_t minor = 0;
    nextPos = data.find( CO_SEPARATOR );
    if( nextPos == std::string::npos || nextPos == 0 )
    {
        LBERROR << "Could not parse node minor version data" << std::endl;
        return false;
    }

    is.clear();
    is.str( data.substr( 0, nextPos ));
    data = data.substr( nextPos + 1 );
    is >> minor;

    if( major != Version::getMajor() || minor != Version::getMinor( ))
    {
        LBWARN << "Protocol mismatch: remote node uses version " << major << '.'
               << minor << ", local node uses " << Version::getMajor() << '.'
               << Version::getMinor() << std::endl;
    }

    // node id
    nextPos = data.find( CO_SEPARATOR );
    if( nextPos == std::string::npos || nextPos == 0 )
    {
        LBERROR << "Could not parse node id data" << std::endl;
        return false;
    }

    _impl->id = data.substr( 0, nextPos );
    data = data.substr( nextPos + 1 );

    // endianness
    nextPos = data.find( CO_SEPARATOR );
    if( nextPos == std::string::npos || nextPos == 0 )
    {
        LBERROR << "Could not parse node endianness data" << std::endl;
        return false;
    }

    is.clear();
    is.str( data.substr( 0, nextPos ));
    data = data.substr( nextPos + 1 );
    is >> _impl->bigEndian;

    // Connections data
    lunchbox::ScopedFastWrite mutex( _impl->connectionDescriptions );
    _impl->connectionDescriptions->clear();
    return co::deserialize( data, _impl->connectionDescriptions.data );
}

bool Node::isBigEndian() const
{
    return _impl->bigEndian;
}

bool Node::isReachable() const
{
    return isListening() || isConnected();
}

bool Node::isConnected() const
{
    return _impl->state == STATE_CONNECTED;
}

bool Node::isClosed() const
{
    return _impl->state == STATE_CLOSED;
}

bool Node::isClosing() const
{
    return _impl->state == STATE_CLOSING;
}

bool Node::isListening() const
{
    return _impl->state == STATE_LISTENING;
}

ConnectionPtr Node::getConnection( const bool preferMulticast )
{
    ConnectionPtr multicast = preferMulticast ? getMulticast() : 0;
    return multicast ? multicast : _impl->outgoing;
}

ConnectionPtr Node::_getConnection( const bool preferMulticast )
{
    ConnectionPtr multicast = preferMulticast ? getMulticast() : 0;
    if( _impl->state != STATE_CLOSED )
        return multicast ? multicast : _impl->outgoing;
    LBUNREACHABLE;
    return 0;
}

ConnectionPtr Node::_getMulticast() const
{
    return _impl->outMulticast.data;
}

OCommand Node::send( const uint32_t cmd, const bool multicast )
{
    ConnectionPtr connection = _getConnection( multicast );
    LBASSERT( connection );
    return OCommand( Connections( 1, connection ), cmd, COMMANDTYPE_NODE );
}

CustomOCommand Node::send( const uint128_t& commandID, const bool multicast )
{
    ConnectionPtr connection = _getConnection( multicast );
    LBASSERT( connection );
    return CustomOCommand( Connections( 1, connection ), commandID );
}

const NodeID& Node::getNodeID() const
{
    return _impl->id;
}

int64_t Node::getLastReceiveTime() const
{
    return _impl->lastReceive;
}

uint32_t Node::getType() const
{
    return _impl->type;
}

void Node::_addMulticast( NodePtr node, ConnectionPtr connection )
{
    lunchbox::ScopedMutex<> mutex( _impl->outMulticast );
    MCData data;
    data.connection = connection;
    data.node = node;
    _impl->multicasts.push_back( data );
}

void Node::_removeMulticast( ConnectionPtr connection )
{
    LBASSERT( connection->getDescription()->type >= CONNECTIONTYPE_MULTICAST );

    lunchbox::ScopedMutex<> mutex( _impl->outMulticast );
    if( _impl->outMulticast == connection )
        _impl->outMulticast.data = 0;
    else
    {
        for( MCDatas::iterator j = _impl->multicasts.begin();
             j != _impl->multicasts.end(); ++j )
        {
            if( (*j).connection != connection )
                continue;

            _impl->multicasts.erase( j );
            return;
        }
    }
}

void Node::_connectMulticast( NodePtr node )
{
    lunchbox::ScopedMutex<> mutex( _impl->outMulticast );

    if( node->_impl->outMulticast.data.isValid( ))
        // multicast already connected by previous _cmdID
        return;

    // Search if the connected node is in the same multicast group as we are
    const ConnectionDescriptions& descriptions = getConnectionDescriptions();
    for( ConnectionDescriptionsCIter i = descriptions.begin();
         i != descriptions.end(); ++i )
    {
        ConnectionDescriptionPtr description = *i;
        if( description->type < CONNECTIONTYPE_MULTICAST )
            continue;

        const ConnectionDescriptions& fromDescs =
            node->getConnectionDescriptions();
        for( ConnectionDescriptionsCIter j = fromDescs.begin();
             j != fromDescs.end(); ++j )
        {
            ConnectionDescriptionPtr fromDescription = *j;
            if( !description->isSameMulticastGroup( fromDescription ))
                continue;

            LBASSERT( !node->_impl->outMulticast.data );
            LBASSERT( node->_impl->multicasts.empty( ));

            if( _impl->outMulticast->isValid() &&
                _impl->outMulticast.data->getDescription() == description )
            {
                node->_impl->outMulticast.data = _impl->outMulticast.data;
                LBINFO << "Using " << description << " as multicast group for "
                       << node->getNodeID() << std::endl;
            }
            // find unused multicast connection to node
            else for( MCDatas::const_iterator k = _impl->multicasts.begin();
                      k != _impl->multicasts.end(); ++k )
            {
                const MCData& data = *k;
                ConstConnectionDescriptionPtr dataDesc =
                    data.connection->getDescription();
                if( !description->isSameMulticastGroup( dataDesc ))
                    continue;

                node->_impl->multicasts.push_back( data );
                LBINFO << "Adding " << dataDesc << " as multicast group for "
                       << node->getNodeID() << std::endl;
            }
        }
    }
}

void Node::_connectMulticast( NodePtr node, ConnectionPtr connection )
{
    lunchbox::ScopedMutex<> mutex( _impl->outMulticast );
    MCDatas::iterator i = node->_impl->multicasts.begin();
    for( ; i != node->_impl->multicasts.end(); ++i )
    {
        if( (*i).connection == connection )
            break;
    }

    if( node->_impl->outMulticast->isValid( ))
    {
        if( node->_impl->outMulticast.data == connection )
        {
            // nop, connection already used
            LBASSERT( i == node->_impl->multicasts.end( ));
        }
        else if( i == node->_impl->multicasts.end( ))
        {
            // another connection is used as multicast connection, save this
            LBASSERT( isListening( ));
            MCData data;
            data.connection = connection;
            data.node = this;
            _impl->multicasts.push_back( data );
        }
        // else nop, already know connection
    }
    else
    {
        node->_impl->outMulticast.data = connection;
        if( i != node->_impl->multicasts.end( ))
            node->_impl->multicasts.erase( i );
    }
}

void Node::_setListening()
{
    _impl->state = STATE_LISTENING;
}

void Node::_setClosing()
{
    _impl->state = STATE_CLOSING;
}

void Node::_setClosed()
{
    _impl->state = STATE_CLOSED;
}

void Node::_connect( ConnectionPtr connection )
{
    _impl->outgoing = connection;
    _impl->state = STATE_CONNECTED;
}

void Node::_disconnect()
{
    _impl->state = STATE_CLOSED;
    _impl->outgoing = 0;
    _impl->outMulticast.data = 0;
    _impl->multicasts.clear();
}

void Node::_setLastReceive( const int64_t time )
{
    _impl->lastReceive = time;
}

std::ostream& operator << ( std::ostream& os, const State state )
{
    os << ( state == STATE_CLOSED ? "closed" :
            state == STATE_CONNECTED ? "connected" :
            state == STATE_LISTENING ? "listening" : "ERROR" );
    return os;
}

std::ostream& operator << ( std::ostream& os, const Node& node )
{
    os << "node " << node.getNodeID() << " " << node._impl->state;
    const ConnectionDescriptions& descs = node.getConnectionDescriptions();
    for( ConnectionDescriptionsCIter i = descs.begin(); i != descs.end(); ++i )
        os << ", " << (*i)->toString();
    return os;
}

}
