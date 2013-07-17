
/* Copyright (c) 2007-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *               2009-2010, Cedric Stalder <cedric.stalder@gmail.com>
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

#include "dataIStream.h"

#include "global.h"
#include "log.h"
#include "node.h"

#include <lunchbox/buffer.h>
#include <lunchbox/debug.h>
#include <lunchbox/decompressor.h>
#include <lunchbox/plugins/compressor.h>

#include <string.h>

namespace co
{
namespace detail
{
class DataIStream
{
public:
    DataIStream( const bool swap_ )
            : input( 0 )
            , inputSize( 0 )
            , position( 0 )
            , swap( swap_ )
        {}

    /** The current input buffer */
    const uint8_t* input;

    /** The size of the input buffer */
    uint64_t inputSize;

    /** The current read position in the buffer */
    uint64_t position;

    lunchbox::Decompressor decompressor; //!< current decompressor
    lunchbox::Bufferb data; //!< decompressed buffer
    bool swap; //!< Invoke endian conversion
};
}

DataIStream::DataIStream( const bool swap_ )
        : _impl( new detail::DataIStream( swap_ ))
{}

DataIStream::DataIStream( const DataIStream& rhs )
        : _impl( new detail::DataIStream( rhs._impl->swap ))
{}

DataIStream::~DataIStream()
{
    _reset();
    delete _impl;
}

DataIStream& DataIStream::operator = ( const DataIStream& rhs )
{
    _reset();
    setSwapping( rhs.isSwapping( ));
    return *this;
}

void DataIStream::setSwapping( const bool onOff )
{
    _impl->swap = onOff;
}

bool DataIStream::isSwapping() const
{
    return _impl->swap;
}

void DataIStream::_reset()
{
    _impl->input     = 0;
    _impl->inputSize = 0;
    _impl->position  = 0;
    _impl->swap      = false;
}

void DataIStream::_read( void* data, uint64_t size )
{
    if( !_checkBuffer( ))
    {
        LBUNREACHABLE;
        LBERROR << "No more input data" << std::endl;
        return;
    }

    LBASSERT( _impl->input );
    if( size > _impl->inputSize - _impl->position )
    {
        LBERROR << "Not enough data in input buffer: need " << size
                << " bytes, " << _impl->inputSize - _impl->position << " left "
                << std::endl;
        LBUNREACHABLE;
        // TODO: Allow reads which are asymmetric to writes by reading from
        // multiple blocks here?
        return;
    }

    memcpy( data, _impl->input + _impl->position, size );
    _impl->position += size;
}

const void* DataIStream::getRemainingBuffer( const uint64_t size )
{
    if( !_checkBuffer( ))
        return 0;

    LBASSERT( _impl->position + size <= _impl->inputSize );
    if( _impl->position + size > _impl->inputSize )
        return 0;

    _impl->position += size;
    return _impl->input + _impl->position - size;
}

uint64_t DataIStream::getRemainingBufferSize()
{
    if( !_checkBuffer( ))
        return 0;

    return _impl->inputSize - _impl->position;
}

bool DataIStream::wasUsed() const
{
    return _impl->input != 0;
}

bool DataIStream::_checkBuffer()
{
    while( _impl->position >= _impl->inputSize )
    {
        uint32_t compressor = EQ_COMPRESSOR_NONE;
        uint32_t nChunks = 0;
        const void* data = 0;

        if( !getNextBuffer( compressor, nChunks, &data, _impl->inputSize ))
            return false;

        _impl->input = _decompress( data, compressor, nChunks,
                                    _impl->inputSize );
        _impl->position = 0;
    }
    return true;
}

const uint8_t* DataIStream::_decompress( const void* data, const uint32_t name,
                                         const uint32_t nChunks,
                                         const uint64_t dataSize )
{
    const uint8_t* src = reinterpret_cast< const uint8_t* >( data );
    if( name == EQ_COMPRESSOR_NONE )
        return src;

    LBASSERT( name > EQ_COMPRESSOR_NONE );
#ifndef CO_AGGRESSIVE_CACHING
    _impl->data.clear();
#endif
    _impl->data.reset( dataSize );

    _impl->decompressor.setup( Global::getPluginRegistry(), name );
    LBASSERT( _impl->decompressor.uses( name ));

    uint64_t outDim[2] = { 0, dataSize };
    uint64_t* chunkSizes = static_cast< uint64_t* >(
                                alloca( nChunks * sizeof( uint64_t )));
    void** chunks = static_cast< void ** >(
                                alloca( nChunks * sizeof( void* )));

    for( uint32_t i = 0; i < nChunks; ++i )
    {
        const uint64_t size = *reinterpret_cast< const uint64_t* >( src );
        chunkSizes[ i ] = size;
        src += sizeof( uint64_t );

        // The plugin API uses non-const source buffers for in-place operations
        chunks[ i ] = const_cast< uint8_t* >( src );
        src += size;
    }

    _impl->decompressor.decompress( chunks, chunkSizes, nChunks,
                                    _impl->data.getData(), outDim );
    return _impl->data.getData();
}

}
