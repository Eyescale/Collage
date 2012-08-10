
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

#include "objectICommand.h"

#include "command.h"


namespace co
{

namespace detail
{

class ObjectICommand
{
public:
    UUID objectID;
    uint32_t instanceID;
};

}

ObjectICommand::ObjectICommand( CommandPtr command )
    : NodeICommand( command )
    , _impl( new detail::ObjectICommand )
{
    *this >> _impl->objectID >> _impl->instanceID;
}

ObjectICommand::~ObjectICommand()
{
    delete _impl;
}

const UUID& ObjectICommand::getObjectID() const
{
    return _impl->objectID;
}

uint32_t ObjectICommand::getInstanceID() const
{
    return _impl->instanceID;
}

}
