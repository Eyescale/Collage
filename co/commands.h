
/* Copyright (c) 2005-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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
        COMMANDTYPE_NODE,
        COMMANDTYPE_OBJECT,
        COMMANDTYPE_CUSTOM = 1<<7,
        COMMANDTYPE_INVALID = 0xFFFFFFFFu //!< @internal
    };

    enum
    {
        CMD_NODE_COMMAND, //!< @internal
        CMD_NODE_INTERNAL, //!< @internal
        CMD_NODE_CUSTOM = 50,  //!< Commands for Node subclasses start here
        CMD_NODE_MAXIMUM = 0xFFFFFFu, //!< Highest allowed node command (2^24-1)
        CMD_OBJECT_CUSTOM = 10, //!< Commands for Object subclasses start here
        CMD_INVALID = 0xFFFFFFFFu //!< @internal
    };
}

namespace lunchbox
{
template<> inline void byteswap( co::CommandType& value )
    { byteswap( reinterpret_cast< uint32_t& >( value )); }
}

#endif // CO_COMMANDS_H
