
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

#include "objectDataOCommand.h"

#include "buffer.h"
#include "objectDataICommand.h"
#include <pression/plugins/compressorTypes.h>

namespace co
{

namespace detail
{

class ObjectDataOCommand
{
public:
    ObjectDataOCommand( co::DataOStream* stream_, const void* data_,
                        const uint64_t size_ )
        : data( data_ )
        , size( 0 )
        , stream( stream_ )
    {
        if( stream )
        {
            size = stream->getCompressedDataSize();
            if( size == 0 )
                size = size_;
        }
    }

    ObjectDataOCommand( const ObjectDataOCommand& rhs )
        : data( rhs.data )
        , size( rhs.size )
        , stream( rhs.stream )
    {}

    const void* const data;
    uint64_t size;
    co::DataOStream* stream;
};

}

ObjectDataOCommand::ObjectDataOCommand( const Connections& receivers,
                                        const uint32_t cmd, const uint32_t type,
                                        const uint128_t& id,
                                        const uint32_t instanceID,
                                        const uint128_t& version,
                                        const uint32_t sequence,
                                        const void* data,
                                        const uint64_t size,
                                        const bool isLast,
                                        DataOStream* stream )
    : ObjectOCommand( receivers, cmd, type, id, instanceID )
    , _impl( new detail::ObjectDataOCommand( stream, data, size ))
{
    _init( version, sequence, size, isLast );
}

ObjectDataOCommand::ObjectDataOCommand( const ObjectDataOCommand& rhs )
    : ObjectOCommand( rhs )
    , _impl( new detail::ObjectDataOCommand( *rhs._impl ))
{
}

void ObjectDataOCommand::_init( const uint128_t& version,
                                const uint32_t sequence,
                                const uint64_t size, const bool isLast )
{
    *this << version << size << sequence << isLast;

    if( _impl->stream )
        _impl->stream->streamDataHeader( *this );
    else
        *this << EQ_COMPRESSOR_NONE << 0u; // compressor, nChunks
}

ObjectDataOCommand::~ObjectDataOCommand()
{
    if( _impl->stream && _impl->size > 0 )
    {
        sendHeader( _impl->size );
        const Connections& connections = getConnections();
        for( ConnectionsCIter i = connections.begin(); i != connections.end();
             ++i )
        {
            ConnectionPtr conn = *i;
            _impl->stream->sendBody( conn, _impl->data, _impl->size );
        }
    }

    delete _impl;
}

ObjectDataICommand ObjectDataOCommand::_getCommand( LocalNodePtr node )
{
    lunchbox::Bufferb& outBuffer = getBuffer();
    uint8_t* bytes = outBuffer.getData();
    reinterpret_cast< uint64_t* >( bytes )[ 0 ] = outBuffer.getSize();

    BufferPtr inBuffer = node->allocBuffer( outBuffer.getSize( ));
    inBuffer->swap( outBuffer );
    return ObjectDataICommand( node, node, inBuffer, false );
}

}
