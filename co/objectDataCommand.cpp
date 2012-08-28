
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
        , sequence( 0 )
        , datasize( 0 )
        , compressor( EQ_COMPRESSOR_NONE )
        , chunks( 1 )
        , isLast( false )
    {}
    uint128_t version;
    uint32_t sequence;
    uint64_t datasize;
    uint32_t compressor;
    uint32_t chunks;
    bool isLast;
};

}

ObjectDataCommand::ObjectDataCommand( BufferPtr buffer )
    : ObjectCommand( buffer )
    , _impl( new detail::ObjectDataCommand )
{
    if( buffer )
        *this >> _impl->version >> _impl->sequence >> _impl->datasize
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

}
