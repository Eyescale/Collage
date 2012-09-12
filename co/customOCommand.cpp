
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "customOCommand.h"


namespace co
{

namespace detail
{

class CustomOCommand
{
public:
    CustomOCommand( const uint128_t& commandID_ )
        : commandID( commandID_ )
    {}

    CustomOCommand( const CustomOCommand& rhs )
        : commandID( rhs.commandID )
    {}

    const uint128_t commandID;
};

}

CustomOCommand::CustomOCommand( const Connections& receivers,
                                const uint128_t& commandID )
    : OCommand( receivers, CMD_NODE_COMMAND, COMMANDTYPE_NODE )
    , _impl( new detail::CustomOCommand( commandID ))
{
    _init();
}

CustomOCommand::CustomOCommand( LocalNodePtr localNode,
                                const uint128_t& commandID )
    : OCommand( localNode.get(), localNode, CMD_NODE_COMMAND, COMMANDTYPE_NODE )
    , _impl( new detail::CustomOCommand( commandID ))
{
    _init();
}

CustomOCommand::CustomOCommand( const CustomOCommand& rhs )
    : OCommand( rhs )
    , _impl( new detail::CustomOCommand( *rhs._impl ))
{
    _init();
}

void CustomOCommand::_init()
{
    *this << _impl->commandID;
}

CustomOCommand::~CustomOCommand()
{
    delete _impl;
}

}
