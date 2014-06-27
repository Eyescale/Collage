
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "objectOCommand.h"


namespace co
{

namespace detail
{

class ObjectOCommand
{
public:
    ObjectOCommand() {}
};

}

ObjectOCommand::ObjectOCommand( const Connections& receivers,
                                const uint32_t cmd, const uint32_t type,
                                const uint128_t& id, const uint32_t instanceID )
    : OCommand( receivers, cmd, type )
    , _impl( new detail::ObjectOCommand )
{
    _init( id, instanceID );
}

ObjectOCommand::ObjectOCommand( Dispatcher* const dispatcher,
                                LocalNodePtr localNode, const uint32_t cmd,
                                const uint32_t type, const uint128_t& id,
                                const uint32_t instanceID )
    : OCommand( dispatcher, localNode, cmd, type )
    , _impl( new detail::ObjectOCommand )
{
    _init( id, instanceID );
}

ObjectOCommand::ObjectOCommand( const ObjectOCommand& rhs )
    : OCommand( rhs )
    , _impl( new detail::ObjectOCommand( *rhs._impl ))
{
}

void ObjectOCommand::_init( const uint128_t& id, const uint32_t instanceID )
{
    *this << id << instanceID;
}

ObjectOCommand::~ObjectOCommand()
{
    delete _impl;
}

}
