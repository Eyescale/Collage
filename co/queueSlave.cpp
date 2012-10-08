
/* Copyright (c) 2011-2012, Stefan Eilemann <eile@eyescale.ch>
 *                    2011, Carsten Rohn <carsten.rohn@rtt.ag>
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

#include "queueSlave.h"

#include "buffer.h"
#include "commandQueue.h"
#include "dataIStream.h"
#include "global.h"
#include "objectOCommand.h"
#include "objectICommand.h"
#include "queueCommand.h"

namespace co
{
namespace detail
{
class QueueSlave
{
public:
    QueueSlave( const uint32_t mark, const uint32_t amount)
            : prefetchMark( mark )
            , prefetchAmount( amount )
            , masterInstanceID( EQ_INSTANCE_ALL )
        {}

    co::CommandQueue queue;

    const uint32_t prefetchMark;
    const uint32_t prefetchAmount;
    uint32_t masterInstanceID;

    NodePtr master;
};
}

QueueSlave::QueueSlave( const uint32_t prefetchMark,
                        const uint32_t prefetchAmount )
        : _impl( new detail::QueueSlave( prefetchMark, prefetchAmount ))
{}

QueueSlave::~QueueSlave()
{
    delete _impl;
}

void QueueSlave::attach( const UUID& id, const uint32_t instanceID )
{
    Object::attach(id, instanceID);
    registerCommand( CMD_QUEUE_ITEM, CommandFunc<Object>(0, 0), &_impl->queue );
    registerCommand( CMD_QUEUE_EMPTY, CommandFunc<Object>(0, 0), &_impl->queue);
}

void QueueSlave::applyInstanceData( co::DataIStream& is )
{
    uint128_t masterNodeID;
    is >> _impl->masterInstanceID >> masterNodeID;

    LBASSERT( masterNodeID != NodeID::ZERO );
    LBASSERT( !_impl->master );
    LocalNodePtr localNode = getLocalNode();
    _impl->master = localNode->connect( masterNodeID );
}

ObjectICommand QueueSlave::pop()
{
    static lunchbox::a_int32_t _request;
    const int32_t request = ++_request;

    while( true )
    {
        const size_t queueSize = _impl->queue.getSize();
        if( queueSize <= _impl->prefetchMark )
        {
            send( _impl->master, CMD_QUEUE_GET_ITEM, _impl->masterInstanceID )
                    << _impl->prefetchAmount << getInstanceID() << request;
        }

        ObjectICommand cmd( _impl->queue.pop( ));
        switch( cmd.getCommand( ))
        {
          case CMD_QUEUE_ITEM:
              return ObjectICommand( cmd );

          default:
              LBUNIMPLEMENTED;
          case CMD_QUEUE_EMPTY:
              if( cmd.get< int32_t >() == request )
                  return ObjectICommand( 0, 0, 0, false );
              // else left-over or not our empty command, discard and retry
              break;
        }
    }
}

}
