
/* Copyright (c) 2011-2012, Stefan Eilemann <eile@eyescale.ch>
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

#ifndef CO_QUEUEPACKETS_H
#define CO_QUEUEPACKETS_H

#include <co/packets.h> // base class
#include <co/queueCommand.h> // CMD enums

namespace co
{
    struct QueueItemPacket : public ObjectPacket
    {
        QueueItemPacket()
            : ObjectPacket()
        {
            command = CMD_QUEUE_ITEM;
            size = sizeof( QueueItemPacket );
        }
    };
}

#endif // CO_QUEUEPACKETS_H

