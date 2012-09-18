
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2012, Stefan.Eilemann@epfl.ch
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

#include "objectDataCommand.h"

#include "buffer.h"
#include "command.h"
#include "plugins/compressorTypes.h"


namespace co
{

namespace detail
{

class ObjectDataCommand
{
public:
    ObjectDataCommand()
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

ObjectDataCommand::ObjectDataCommand( const Command& command )
    : ObjectCommand( command )
    , _impl( new detail::ObjectDataCommand )
{
    _init();
}

ObjectDataCommand::ObjectDataCommand( LocalNodePtr local, NodePtr remote,
                                      ConstBufferPtr buffer, const bool swap_ )
    : ObjectCommand( local, remote, buffer, swap_ )
    , _impl( new detail::ObjectDataCommand )
{
    _init();
}


ObjectDataCommand::ObjectDataCommand( const ObjectDataCommand& rhs )
    : ObjectCommand( rhs )
    , _impl( new detail::ObjectDataCommand( *rhs._impl ))
{
    _init();
}

void ObjectDataCommand::_init()
{
    if( isValid( ))
        *this >> _impl->version >> _impl->datasize >> _impl->sequence
              >> _impl->isLast >> _impl->compressor >> _impl->chunks;
}

ObjectDataCommand::~ObjectDataCommand()
{
    delete _impl;
}

uint128_t ObjectDataCommand::getVersion() const
{
    return _impl->version;
}

uint32_t ObjectDataCommand::getSequence() const
{
    return _impl->sequence;
}

uint64_t ObjectDataCommand::getDataSize() const
{
    return _impl->datasize;
}

uint32_t ObjectDataCommand::getCompressor() const
{
    return _impl->compressor;
}

uint32_t ObjectDataCommand::getChunks() const
{
    return _impl->chunks;
}

bool ObjectDataCommand::isLast() const
{
    return _impl->isLast;
}

std::ostream& operator << ( std::ostream& os, const ObjectDataCommand& command )
{
    os << static_cast< const ObjectCommand& >( command );
    if( command.isValid( ))
    {
        os << " v" << command.getVersion() << " size " << command.getDataSize()
           << " seq " << command.getSequence() << " last " << command.isLast();
    }
    return os;
}

}
