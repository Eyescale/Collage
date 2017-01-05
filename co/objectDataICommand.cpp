
/* Copyright (c) 2012-2017, Daniel Nachbaur <danielnachbaur@gmail.com>
 *                          Stefan.Eilemann@epfl.ch
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

#include "objectDataICommand.h"

#include "buffer.h"
#include <pression/data/CompressorInfo.h>
#include <pression/data/Registry.h>

namespace co
{
namespace detail
{

class ObjectDataICommand
{
public:
    ObjectDataICommand()
        : version( 0, 0 )
        , datasize( 0 )
        , sequence( 0 )
        , chunks( 1 )
        , isLast( false )
    {}

    uint128_t version;
    uint64_t datasize;
    uint32_t sequence;
    std::string compressorName;
    uint32_t chunks;
    bool isLast;
};

}

ObjectDataICommand::ObjectDataICommand( const ICommand& command )
    : ObjectICommand( command )
    , _impl( new detail::ObjectDataICommand )
{
    _init();
}

ObjectDataICommand::ObjectDataICommand( LocalNodePtr local, NodePtr remote,
                                        ConstBufferPtr buffer )
    : ObjectICommand( local, remote, buffer )
    , _impl( new detail::ObjectDataICommand )
{
    _init();
}


ObjectDataICommand::ObjectDataICommand( const ObjectDataICommand& rhs )
    : ObjectICommand( rhs )
    , _impl( new detail::ObjectDataICommand( *rhs._impl ))
{
    _init();
}

void ObjectDataICommand::_init()
{
    if( isValid( ))
        *this >> _impl->version >> _impl->datasize >> _impl->sequence
              >> _impl->isLast >> _impl->compressorName >> _impl->chunks;
}

ObjectDataICommand::~ObjectDataICommand()
{
    delete _impl;
}

uint128_t ObjectDataICommand::getVersion() const
{
    return _impl->version;
}

uint32_t ObjectDataICommand::getSequence() const
{
    return _impl->sequence;
}

uint64_t ObjectDataICommand::getDataSize() const
{
    return _impl->datasize;
}

CompressorInfo ObjectDataICommand::getCompressorInfo() const
{
    return pression::data::Registry::getInstance().find( _impl->compressorName);
}

uint32_t ObjectDataICommand::getChunks() const
{
    return _impl->chunks;
}

bool ObjectDataICommand::isLast() const
{
    return _impl->isLast;
}

std::ostream& operator << ( std::ostream& os, const ObjectDataICommand& command )
{
    os << static_cast< const ObjectICommand& >( command );
    if( command.isValid( ))
    {
        os << " v" << command.getVersion() << " size " << command.getDataSize()
           << " seq " << command.getSequence() << " last " << command.isLast();
    }
    return os;
}

}
