
/* Copyright (c) 2011-2012, Stefan Eilemann <eile@eyescale.ch> 
 *               2011, Carsten Rohn <carsten.rohn@rtt.ag> 
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

#include "queueMaster.h"

#include "command.h"
#include "commandCache.h"
#include "dataOStream.h"
#include "objectICommand.h"
#include "nodeOCommand.h"
#include "queuePackets.h"

#include <lunchbox/mtQueue.h>

namespace co
{
namespace detail
{
class QueueMaster : public co::Dispatcher
{
public:
    /** The command handler functions. */
    bool cmdGetItem( Command& command )
    {
        co::ObjectICommand stream( &command );
        uint32_t itemsRequested;
        uint32_t slaveInstanceID;
        int32_t requestID;
        stream >> itemsRequested >> slaveInstanceID >> requestID;

        Commands commands;
        queue.tryPop( itemsRequested, commands );

        for( CommandsCIter i = commands.begin(); i != commands.end(); ++i )
        {
            CommandPtr item = *i;
            ObjectPacket* reply = item->getModifiable< ObjectPacket >();
            reply->instanceID = slaveInstanceID;
            command.getNode()->send( *reply );
        }

        if( itemsRequested > commands.size( ))
            command.getNode()->send( CMD_QUEUE_EMPTY, PACKETTYPE_CO_OBJECT )
                    << stream.getObjectID() << slaveInstanceID << requestID;
        return true;
    }

    typedef lunchbox::MTQueue< CommandPtr > PacketQueue;

    PacketQueue queue;
    co::CommandCache cache;
};
}

QueueMaster::QueueMaster()
        : _impl( new detail::QueueMaster )
{
}

QueueMaster::~QueueMaster()
{
    clear();
    delete _impl;
}

void QueueMaster::attach( const UUID& id, const uint32_t instanceID )
{
    Object::attach( id, instanceID );

    CommandQueue* queue = getLocalNode()->getCommandThreadQueue();
    registerCommand( CMD_QUEUE_GET_ITEM, 
                     CommandFunc< detail::QueueMaster >(
                         _impl, &detail::QueueMaster::cmdGetItem ), queue );
}

void QueueMaster::clear()
{
    _impl->queue.clear();
    _impl->cache.flush();
}

void QueueMaster::getInstanceData( co::DataOStream& os )
{
    os << getInstanceID() << getLocalNode()->getNodeID();
}

void QueueMaster::push( const QueueItemPacket& packet )
{
    LBASSERT( packet.size >= sizeof( QueueItemPacket ));
    LBASSERT( packet.command == CMD_QUEUE_ITEM );

    CommandPtr command = _impl->cache.alloc( getLocalNode(), getLocalNode(),
                                             packet.size );
    QueueItemPacket* queuePacket = command->getModifiable< QueueItemPacket >();

    memcpy( queuePacket, &packet, packet.size );
    queuePacket->objectID = getID();
    _impl->queue.push( command );
}

} // co
