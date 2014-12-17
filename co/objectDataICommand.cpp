
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2012-2013, Stefan.Eilemann@epfl.ch
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
#include <pression/plugins/compressorTypes.h>

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
        , compressor( EQ_COMPRESSOR_NONE )
        , chunks( 1 )
        , isLast( false )
    {}

    uint128_t version;
    uint64_t datasize;
    uint32_t sequence;
    uint32_t compressor;
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
                                      ConstBufferPtr buffer, const bool swap_ )
    : ObjectICommand( local, remote, buffer, swap_ )
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
              >> _impl->isLast >> _impl->compressor >> _impl->chunks;
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

uint32_t ObjectDataICommand::getCompressor() const
{
    return _impl->compressor;
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
