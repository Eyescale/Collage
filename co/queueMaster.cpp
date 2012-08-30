
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

#include "buffer.h"
#include "command.h"
#include "commandCache.h"
#include "dataOStream.h"
#include "objectCommand.h"
#include "objectOCommand.h"
#include "queueCommand.h"
#include "queueItem.h"

#include <lunchbox/mtQueue.h>

namespace co
{

namespace detail
{

class ItemBuffer : public lunchbox::Bufferb, public lunchbox::Referenced
{
public:
    ItemBuffer( lunchbox::Bufferb& from )
        : lunchbox::Bufferb( from )
        , lunchbox::Referenced()
    {}

    ~ItemBuffer()
    {}
};

typedef lunchbox::RefPtr< ItemBuffer > ItemBufferPtr;

class QueueMaster : public co::Dispatcher
{
public:
    QueueMaster( const UUID& masterID_ )
        : co::Dispatcher()
        , masterID( masterID_ )
    {}

    /** The command handler functions. */
    bool cmdGetItem( co::Command& comd )
    {
        co::ObjectCommand command( comd );

        const uint32_t itemsRequested = command.get< uint32_t >();
        const uint32_t slaveInstanceID = command.get< uint32_t >();
        const int32_t requestID = command.get< int32_t >();

        typedef std::vector< ItemBufferPtr > Items;
        Items items;
        queue.tryPop( itemsRequested, items );

        for( Items::const_iterator i = items.begin(); i != items.end(); ++i )
        {
            Connections connections( 1, command.getNode()->getConnection( ));
            co::ObjectOCommand cmd( connections, CMD_QUEUE_ITEM,
                                    COMMANDTYPE_CO_OBJECT, masterID,
                                    slaveInstanceID );

            const ItemBufferPtr item = *i;
            if( !item->isEmpty( ))
                cmd << Array< const void >( item->getData(), item->getSize( ));
        }

        if( itemsRequested > items.size( ))
            command.getNode()->send( CMD_QUEUE_EMPTY, COMMANDTYPE_CO_OBJECT )
                    << command.getObjectID() << slaveInstanceID << requestID;
        return true;
    }

    typedef lunchbox::MTQueue< ItemBufferPtr > ItemQueue;

    const UUID& masterID;
    ItemQueue queue;
    co::CommandCache cache;
};
}

QueueMaster::QueueMaster()
    : _impl( new detail::QueueMaster( getID( )) )
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

QueueItem QueueMaster::push()
{
    return QueueItem( *this );
}

void QueueMaster::_addItem( QueueItem& item )
{
    detail::ItemBufferPtr newBuffer = new detail::ItemBuffer( item.getBuffer());
    _impl->queue.push( newBuffer );
}

} // co
