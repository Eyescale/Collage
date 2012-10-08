
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

#include "customCommand.h"

namespace co
{

namespace detail
{

class CustomCommand
{
public:
    CustomCommand() {}

    uint128_t commandID;
};

}

CustomCommand::CustomCommand( const ICommand& command )
    : ICommand( command )
    , _impl( new detail::CustomCommand )
{
    _init();
}

CustomCommand::CustomCommand( const CustomCommand& rhs )
    : ICommand( rhs )
    , _impl( new detail::CustomCommand )
{
    _init();
}

void CustomCommand::_init()
{
    if( isValid( ))
        *this >> _impl->commandID;
}

CustomCommand::~CustomCommand()
{
    delete _impl;
}

const uint128_t& CustomCommand::getCommandID() const
{
    return _impl->commandID;
}

std::ostream& operator << ( std::ostream& os, const CustomCommand& command )
{
    os << static_cast< const ICommand& >( command );
    if( command.isValid( ))
        os << " custom command " << command.getCommandID();
    return os;
}

}
