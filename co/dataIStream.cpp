
/* Copyright (c) 2007-2017, Stefan Eilemann <eile@equalizergraphics.com>
 *                          Cedric Stalder <cedric.stalder@gmail.com>
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

#include <pression/data/Compressor.h>
#include <pression/data/CompressorInfo.h>
#include <lunchbox/buffer.h>
#include <lunchbox/debug.h>

#include <string.h>

namespace co
{
namespace detail
{
class DataIStream
{
public:
    DataIStream()
        : input( 0 )
        , inputSize( 0 )
        , position( 0 )
    {}

    void initCompressor( const CompressorInfo& info )
    {
        if( info == compressorInfo )
            return;
        compressorInfo = info;
        compressor.reset( info.create( ));
        LBLOG( LOG_OBJECTS ) << "Allocated " << compressorInfo.name <<std::endl;
    }

    /** The current input buffer */
    const uint8_t* input;

    /** The size of the input buffer */
    uint64_t inputSize;

    /** The current read position in the buffer */
    uint64_t position;

    CompressorPtr compressor; //!< current decompressor
    CompressorInfo compressorInfo; //!< current decompressor data
    lunchbox::Bufferb data; //!< decompressed buffer
};
}

DataIStream::DataIStream()
        : _impl( new detail::DataIStream( ))
{}

DataIStream::~DataIStream()
{
    _reset();
    delete _impl;
}

void DataIStream::_reset()
{
    _impl->input     = 0;
    _impl->inputSize = 0;
    _impl->position  = 0;
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
        LBERROR << "Not enough data in input buffer: need 0x" << std::hex << size
                << " bytes, 0x" << _impl->inputSize - _impl->position << " left "
                << std::dec << std::endl;
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
        CompressorInfo info;
        uint32_t nChunks = 0;
        const void* data = 0;

        _impl->position = 0;
        _impl->input = 0;
        _impl->inputSize = 0;

        if( !getNextBuffer( info, nChunks, data, _impl->inputSize ))
            return false;

        _impl->input = _decompress( data, info, nChunks, _impl->inputSize );
    }
    return true;
}

const uint8_t* DataIStream::_decompress( const void* data,
                                         const CompressorInfo& info,
                                         const uint32_t nChunks,
                                         const uint64_t dataSize )
{
    const uint8_t* src = reinterpret_cast< const uint8_t* >( data );
    if( info.name.empty( ))
        return src;

    LBASSERT( !info.name.empty( ));
#ifndef CO_AGGRESSIVE_CACHING
    _impl->data.clear();
#endif
    _impl->data.reset( dataSize );
    _impl->initCompressor( info );

    std::vector< std::pair< const uint8_t*, size_t >> inputs( nChunks );
    for( uint32_t i = 0; i < nChunks; ++i )
    {
        const uint64_t size = *reinterpret_cast< const uint64_t* >( src );
        src += sizeof( uint64_t );

        inputs[ i ] = { src, size };
        src += size;
    }

    _impl->compressor->decompress( inputs, _impl->data.getData(), dataSize );
    return _impl->data.getData();
}

}
