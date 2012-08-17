
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

#ifndef CO_PACKETS_H
#define CO_PACKETS_H

#include <co/commands.h> // used for CMD_ enums
#include <co/types.h>
#include <co/version.h>  // enum
#include <lunchbox/uuid.h> // member

namespace co
{
    enum PacketType
    {
        PACKETTYPE_CO_NODE,
        PACKETTYPE_CO_OBJECT,
        PACKETTYPE_CO_CUSTOM = 1<<7
    };

    /** A packet send over the network. */
    struct Packet
    {
        uint64_t size; //!< Total size, set by the most specific sub-struct
        uint32_t type; //!< The packet (receiver) type
        uint32_t command; //!< The specific command name @sa commands.h
        
#if 0
        union
        {
            Foo foo;
            uint64_t paddingPacket; // pad to multiple-of-8
        };
#endif
    };

    // String transmission: the packets define a 8-char string at the end of the
    // packet. When the packet is sent using Node::send( Packet&, string& ), the
    // whole string is appended to the packet, so that the receiver has to do
    // nothing special to receive and use the full packet.


    //------------------------------------------------------------
    // ostream operators
    //------------------------------------------------------------
    inline std::ostream& operator << ( std::ostream& os, 
                                       const Packet* packet )
    {
        os << "packet dt " << packet->type << " cmd "
           << packet->command;
        return os;
    }
}

namespace lunchbox
{
template<> inline void byteswap( co::PacketType& value )
    { byteswap( reinterpret_cast< uint32_t& >( value )); }
}

#endif // CO_PACKETS_H

