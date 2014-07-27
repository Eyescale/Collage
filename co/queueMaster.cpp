
/* Copyright (c) 2011-2014, Stefan Eilemann <eile@eyescale.ch>
 *                    2011, Carsten Rohn <carsten.rohn@rtt.ag>
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

#include "queueMaster.h"

#include "dataOStream.h"
#include "objectICommand.h"
#include "objectOCommand.h"
#include "queueCommand.h"
#include "queueItem.h"

#include <lunchbox/buffer.h>
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
    QueueMaster( const co::QueueMaster& parent )
        : co::Dispatcher()
        , _parent( parent )
    {}

    /** The command handler functions. */
    bool cmdGetItem( co::ICommand& comd )
    {
        co::ObjectICommand command( comd );

        const uint32_t itemsRequested = command.get< uint32_t >();
        const uint32_t slaveInstanceID = command.get< uint32_t >();
        const int32_t requestID = command.get< int32_t >();

        typedef std::vector< ItemBufferPtr > Items;
        Items items;
        queue.tryPop( itemsRequested, items );

        Connections connections( 1, command.getNode()->getConnection( ));
        for( Items::const_iterator i = items.begin(); i != items.end(); ++i )
        {
            co::ObjectOCommand cmd( connections, CMD_QUEUE_ITEM,
                                    COMMANDTYPE_OBJECT, _parent.getID(),
                                    slaveInstanceID );

            const ItemBufferPtr item = *i;
            if( !item->isEmpty( ))
                cmd << Array< const void >( item->getData(), item->getSize( ));
        }

        if( itemsRequested > items.size( ))
            co::ObjectOCommand( connections, CMD_QUEUE_EMPTY,
                                COMMANDTYPE_OBJECT, command.getObjectID(),
                                slaveInstanceID ) << requestID;
        return true;
    }

    typedef lunchbox::MTQueue< ItemBufferPtr > ItemQueue;

    ItemQueue queue;

private:
    const co::QueueMaster& _parent;
};
}

QueueMaster::QueueMaster()
#pragma warning(push)
#pragma warning(disable: 4355)
    : _impl( new detail::QueueMaster( *this ))
#pragma warning(pop)
{
}

QueueMaster::~QueueMaster()
{
    clear();
    delete _impl;
}

void QueueMaster::attach( const uint128_t& id, const uint32_t instanceID )
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
