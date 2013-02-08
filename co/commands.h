
/* Copyright (c) 2005-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_COMMANDS_H
#define CO_COMMANDS_H

#include <lunchbox/bitOperation.h> // byteswap inline impl

namespace co
{
    enum CommandType
    {
        COMMANDTYPE_NODE, //!< A Node/LocalNode command
        COMMANDTYPE_OBJECT, //!< An Object command
        COMMANDTYPE_CUSTOM = 1<<7, // !< Application-specific command
        COMMANDTYPE_INVALID = 0xFFFFFFFFu //!< @internal
    };

    enum Commands
    {
        CMD_NODE_CUSTOM = 50,  //!< Commands for Node subclasses start here
        CMD_NODE_MAXIMUM = 0xFFFFFFu, //!< Highest allowed node command (2^24-1)
        CMD_OBJECT_CUSTOM = 10, //!< Commands for Object subclasses start here
        CMD_INVALID = 0xFFFFFFFFu //!< @internal
    };

    /** @internal Minimal packet size sent by DataOStream / read by LocalNode */
    static const size_t COMMAND_MINSIZE = 256;

    /** @internal Minimal allocation size of a packet. */
    static const size_t COMMAND_ALLOCSIZE = 4096; // Bigger than minSize!
}

namespace lunchbox
{
template<> inline void byteswap( co::CommandType& value )
    { byteswap( reinterpret_cast< uint32_t& >( value )); }
}

#endif // CO_COMMANDS_H
