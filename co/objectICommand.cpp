
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2012-2014, Stefan.Eilemann@epfl.ch
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

#include "objectICommand.h"

#include "buffer.h"


namespace co
{
namespace detail
{

class ObjectICommand
{
public:
    ObjectICommand()
    {}

    ObjectICommand( const ObjectICommand& rhs )
        : objectID( rhs.objectID )
        , instanceID( rhs.instanceID )
    {}

    uint128_t objectID;
    uint32_t instanceID;
};
}

ObjectICommand::ObjectICommand( LocalNodePtr local, NodePtr remote,
                                ConstBufferPtr buffer, const bool swap_ )
    : ICommand( local, remote, buffer, swap_ )
    , _impl( new detail::ObjectICommand )
{
    _init();
}

ObjectICommand::ObjectICommand( const ICommand& command )
    : ICommand( command )
    , _impl( new detail::ObjectICommand )
{
    _init();
}

ObjectICommand::ObjectICommand( const ObjectICommand& rhs )
    : ICommand( rhs )
    , _impl( new detail::ObjectICommand( *rhs._impl ))
{
    _init();
}

void ObjectICommand::_init()
{
    if( isValid( ))
        *this >> _impl->objectID >> _impl->instanceID;
}

ObjectICommand::~ObjectICommand()
{
    delete _impl;
}

const uint128_t& ObjectICommand::getObjectID() const
{
    return _impl->objectID;
}

uint32_t ObjectICommand::getInstanceID() const
{
    return _impl->instanceID;
}

std::ostream& operator << ( std::ostream& os, const ObjectICommand& command )
{
    os << static_cast< const ICommand& >( command );
    if( command.isValid( ))
    {
        os << " object " << command.getObjectID()
           << "." << command.getInstanceID();
    }
    return os;
}

}
