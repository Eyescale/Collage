
/* Copyright (c) 2006-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2011, Cedric Stalder <cedric.stalder@gmail.com>
 *               2012-2014, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "barrier.h"

#include "iCommand.h"
#include "connection.h"
#include "dataIStream.h"
#include "dataOStream.h"
#include "global.h"
#include "log.h"
#include "objectICommand.h"
#include "objectOCommand.h"
#include "barrierCommand.h"
#include "exception.h"

#include <lunchbox/monitor.h>
#include <lunchbox/stdExt.h>

namespace co
{
namespace
{
struct Request
{
    Request()
            : time( 0 ), timeout( LB_TIMEOUT_INDEFINITE ), incarnation( 0 ) {}
    uint64_t time;
    uint32_t timeout;
    uint32_t incarnation;
    Nodes nodes;
};

typedef stde::hash_map< uint128_t, Request > RequestMap;
typedef RequestMap::iterator RequestMapIter;
}

namespace detail
{
class Barrier
{
public:
    Barrier() : height( 0 ) {}
    Barrier( const uint128_t& masterID_, const uint32_t height_ )
        : masterID( masterID_ )
        , height( height_ )
    {}

    /** The master barrier node. */
    NodeID masterID;

    /** The height of the barrier, only set on the master. */
    uint32_t height;

    /** The local, connected instantiation of the master node. */
    NodePtr master;

    /** Slave nodes which have entered the barrier, index per version. */
    RequestMap enteredNodes;

    /** The monitor used for barrier leave notification. */
    lunchbox::Monitor< uint32_t > incarnation;
};
}

typedef CommandFunc<Barrier> CmdFunc;

#ifdef COLLAGE_V1_API
Barrier::Barrier( NodePtr master, const uint32_t height )
    : _impl( new detail::Barrier( master ? master->getNodeID() : NodeID(),
                                  height ))
{}
#endif

Barrier::Barrier( LocalNodePtr localNode, const uint128_t& masterNodeID,
                  const uint32_t height )
    : _impl( new detail::Barrier( masterNodeID.isUUID() ?
                                  masterNodeID : localNode->getNodeID(),
                                  height ))
{
    localNode->registerObject( this );
}

Barrier::Barrier( LocalNodePtr localNode, const ObjectVersion& id )
    : _impl( new detail::Barrier )
{
    localNode->mapObject( this, id );
}

Barrier::~Barrier()
{
    LocalNodePtr localNode = getLocalNode();
    if( localNode )
        localNode->releaseObject( this );

    delete _impl;
}

//---------------------------------------------------------------------------
// Serialization
//---------------------------------------------------------------------------
void Barrier::getInstanceData( DataOStream& os )
{
    LBASSERT( _impl->masterID != NodeID( ));
    os << _impl->height << _impl->masterID;
}

void Barrier::applyInstanceData( DataIStream& is )
{
    is >> _impl->height >> _impl->masterID;
}

void Barrier::pack( DataOStream& os )
{
    os << _impl->height;
}

void Barrier::unpack( DataIStream& is )
{
    is >> _impl->height;
}
//---------------------------------------------------------------------------

void Barrier::setHeight( const uint32_t height )
{
    _impl->height = height;
}

void Barrier::increase()
{
    ++_impl->height;
}

uint32_t Barrier::getHeight() const
{
    return _impl->height;
}

void Barrier::attach( const uint128_t& id, const uint32_t instanceID )
{
    Object::attach( id, instanceID );

    LocalNodePtr node = getLocalNode();
    CommandQueue* queue = node->getCommandThreadQueue();

    registerCommand( CMD_BARRIER_ENTER,
                     CmdFunc( this, &Barrier::_cmdEnter ), queue );
    registerCommand( CMD_BARRIER_ENTER_REPLY,
                     CmdFunc( this, &Barrier::_cmdEnterReply ), queue );

#ifdef COLLAGE_V1_API
    if( _impl->masterID == NodeID( ))
        _impl->masterID = node->getNodeID();
#else
    LBASSERT( _impl->masterID == NodeID( ));
#endif
}

void Barrier::enter( const uint32_t timeout )
{
    LBASSERT( _impl->height > 0 );
    LBASSERT( _impl->masterID != NodeID( ));

    if( _impl->height == 1 ) // trivial ;)
        return;

    if( !_impl->master )
    {
        LocalNodePtr localNode = getLocalNode();
        _impl->master = localNode->connect( _impl->masterID );
    }

    LBASSERT( _impl->master );
    LBASSERT( _impl->master->isReachable( ));
    if( !_impl->master || !_impl->master->isReachable( ))
    {
        LBWARN << "Can't connect barrier master node " << _impl->masterID
               << std::endl;
        return;
    }

    LBLOG( LOG_BARRIER ) << "enter barrier " << getID() << " v" << getVersion()
                         << ", height " << _impl->height << std::endl;

    const uint32_t leaveVal = _impl->incarnation.get() + 1;

    send( _impl->master, CMD_BARRIER_ENTER )
        << getVersion() << leaveVal - 1 << timeout;

    if( timeout == LB_TIMEOUT_INDEFINITE )
        _impl->incarnation.waitEQ( leaveVal );
    else if( !_impl->incarnation.timedWaitEQ( leaveVal, timeout ))
        throw Exception( Exception::TIMEOUT_BARRIER );

    LBLOG( LOG_BARRIER ) << "left barrier " << getID() << " v" << getVersion()
                         << ", height " << _impl->height << std::endl;
}

bool Barrier::_cmdEnter( ICommand& cmd )
{
    LB_TS_THREAD( _thread );
    LBASSERTINFO( !_impl->master || _impl->master == getLocalNode(),
                  _impl->master );

    ObjectICommand command( cmd );
    const uint128_t version = command.get< uint128_t >();
    const uint32_t incarnation = command.get< uint32_t >();
    const uint32_t timeout = command.get< uint32_t >();

    LBLOG( LOG_BARRIER ) << "handle barrier enter " << command
                         << " v" << version
                         << " barrier v" << getVersion() << std::endl;

    Request& request = _impl->enteredNodes[ version ];

    LBLOG( LOG_BARRIER ) << "enter barrier v" << version
                         << ", has " << request.nodes.size() << " of "
                         << _impl->height << std::endl;

    request.time = getLocalNode()->getTime64();

    // It's the first call to enter barrier
    if( request.nodes.empty( ))
    {
        request.incarnation = incarnation;
        request.timeout = timeout;
    }
    else if( request.timeout != LB_TIMEOUT_INDEFINITE )
    {
        // the incarnation belongs to an older barrier
        if( request.incarnation < incarnation )
        {
            // send directly the reply command to unblock the caller
            _sendNotify( version, command.getNode( ));
            return true;
        }
        // the previous enter had a timeout, start a new synchronization
        //  (same version means same group -> no member can run ahead)
        else if( request.incarnation > incarnation )
        {
            request.nodes.clear();
            request.incarnation = incarnation;
            request.timeout = timeout;
        }
    }
    request.nodes.push_back( command.getNode( ));

    // clean older data which was not removed during older synchronization
    if( request.timeout != LB_TIMEOUT_INDEFINITE )
        _cleanup( request.time );

    // If we got early entry requests for this barrier, just note their
    // appearance. This requires that another request for the later version
    // arrives once the barrier reaches this version. The only case when this is
    // not the case is when no contributor to the current version contributes to
    // the later version, in which case deadlocks might happen because the later
    // version never leaves the barrier. We simply assume this is not the case.
    if( version > getVersion( ))
        return true;

    // If it's an older version a timeout has been handled.
    // For performance, send directly the order to unblock the caller.
    if( timeout != LB_TIMEOUT_INDEFINITE && version < getVersion( ))
    {
        LBASSERT( incarnation == 0 );
        _sendNotify( version, command.getNode( ) );
        return true;
    }

    LBASSERTINFO( version == getVersion(),
                  "Barrier master updated to new version while in barrier " <<
                  getID() << " (" << version << " != " << getVersion() << ")" );

    Nodes& nodes = request.nodes;
    if( nodes.size() < _impl->height )
        return true;

    LBASSERT( nodes.size() == _impl->height );
    LBLOG( LOG_BARRIER ) << "Barrier reached " << getID() << " v" << version
                         << std::endl;

    stde::usort( nodes );

    for( NodesIter i = nodes.begin(); i != nodes.end(); ++i )
        _sendNotify( version, *i );

    // delete node vector for version
    RequestMapIter i = _impl->enteredNodes.find( version );
    LBASSERT( i != _impl->enteredNodes.end( ));
    _impl->enteredNodes.erase( i );
    return true;
}

void Barrier::_sendNotify( const uint128_t& version, NodePtr node )
{
    LB_TS_THREAD( _thread );
    LBASSERTINFO( !_impl->master || _impl->master == getLocalNode(),
                  _impl->master );

    if( node->isLocal( )) // OPT
    {
        LBLOG( LOG_BARRIER ) << "Unlock local user(s)" << std::endl;
        // the case where we receive a different version of the barrier meant
        // that previosly we have detect a timeout true negative
        if( version == getVersion() )
            ++_impl->incarnation;
    }
    else
    {
        LBLOG( LOG_BARRIER ) << "Unlock " << node << std::endl;
        send( node, CMD_BARRIER_ENTER_REPLY ) << version;
    }
}

void Barrier::_cleanup( const uint64_t time )
{
    LB_TS_THREAD( _thread );
    LBASSERTINFO( !_impl->master || _impl->master == getLocalNode(),
                  _impl->master );

    if( _impl->enteredNodes.size() < 2 )
        return;

    for( RequestMapIter i = _impl->enteredNodes.begin();
         i != _impl->enteredNodes.end(); ++i )
    {
        Request& cleanNodes = i->second;

        if( cleanNodes.timeout == LB_TIMEOUT_INDEFINITE )
            continue;

        const uint32_t timeout = cleanNodes.timeout != LB_TIMEOUT_DEFAULT ?
                        cleanNodes.timeout :
                        Global::getIAttribute( Global::IATTR_TIMEOUT_DEFAULT );

        if( time > cleanNodes.time + timeout )
        {
            _impl->enteredNodes.erase( i );
            return;
        }
    }
}

bool Barrier::_cmdEnterReply( ICommand& cmd )
{
    ObjectICommand command( cmd );
    LB_TS_THREAD( _thread );
    LBLOG( LOG_BARRIER ) << "Got ok, unlock local user(s)" << std::endl;
    const uint128_t version = command.get< uint128_t >();

    if( version == getVersion( ))
        ++_impl->incarnation;

    return true;
}

}
