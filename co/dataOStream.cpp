
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "dataOStream.h"

#include "buffer.h"
#include "connectionDescription.h"
#include "connections.h"
#include "cpuCompressor.h"
#include "global.h"
#include "log.h"
#include "node.h"
#include "types.h"


namespace co
{
namespace
{
//#define EQ_INSTRUMENT_DATAOSTREAM
#ifdef EQ_INSTRUMENT_DATAOSTREAM
lunchbox::a_int32_t nBytes;
lunchbox::a_int32_t nBytesIn;
lunchbox::a_int32_t nBytesOut;
CO_API lunchbox::a_int32_t nBytesSaved;
CO_API lunchbox::a_int32_t nBytesSent;
lunchbox::a_int32_t compressionTime;
#endif

enum CompressorState
{
    STATE_UNCOMPRESSED,
    STATE_PARTIAL,
    STATE_COMPLETE,
    STATE_UNCOMPRESSIBLE
};
}

namespace detail
{
class DataOStream
{
public:
    CompressorState state;

    /** The buffer used for saving and buffering */
    lunchbox::Bufferb buffer;

    /** The start position of the buffering, always 0 if !_save */
    uint64_t bufferStart;

    /** The uncompressed size of a completely compressed buffer. */
    uint64_t dataSize;

    /** The compressed size, 0 for uncompressed or uncompressable data. */
    uint64_t compressedDataSize;

    /** Locked connections to the receivers, if _enabled */
    Connections connections;

    /** The compressor instance. */
    CPUCompressor compressor;

    /** The output stream is enabled for writing */
    bool enabled;

    /** Some data has been sent since it was enabled */
    bool dataSent;

    /** Save all sent data */
    bool save;

    DataOStream()
            : state( STATE_UNCOMPRESSED )
            , bufferStart( 0 )
            , dataSize( 0 )
            , enabled( false )
            , dataSent( false )
            , save( false )
        {}

    uint32_t getCompressor() const
    {
        if( state == STATE_UNCOMPRESSED || state == STATE_UNCOMPRESSIBLE )
            return EQ_COMPRESSOR_NONE;
        return compressor.getName();
    }

    uint32_t getNumChunks() const
    {
        if( state == STATE_UNCOMPRESSED || state == STATE_UNCOMPRESSIBLE )
            return 1;
        return compressor.getNumResults();
    }


    /** Compress data and update the compressor state. */
    void compress( void* src, const uint64_t size, const CompressorState result)
    {
        if( state == result || state == STATE_UNCOMPRESSIBLE )
            return;
#ifdef EQ_INSTRUMENT_DATAOSTREAM
        nBytesIn += size;
#endif
        const uint64_t threshold =
           uint64_t( Global::getIAttribute( Global::IATTR_OBJECT_COMPRESSION ));

        if( !compressor.isValid( compressor.getName( )) || size <= threshold )
        {
            state = STATE_UNCOMPRESSED;
            return;
        }

        const uint64_t inDims[2] = { 0, size };

#ifdef EQ_INSTRUMENT_DATAOSTREAM
        lunchbox::Clock clock;
#endif
        compressor.compress( src, inDims );
#ifdef EQ_INSTRUMENT_DATAOSTREAM
        compressionTime += uint32_t( clock.getTimef() * 1000.f );
#endif

        const uint32_t nChunks = compressor.getNumResults();
        compressedDataSize = 0;
        LBASSERT( nChunks > 0 );

        for( uint32_t i = 0; i < nChunks; ++i )
        {
            void* chunk;
            uint64_t chunkSize;

            compressor.getResult( i, &chunk, &chunkSize );
            compressedDataSize += chunkSize;
        }
#ifdef EQ_INSTRUMENT_DATAOSTREAM
        nBytesOut += compressedDataSize;
#endif

        if( compressedDataSize >= size )
        {
            state = STATE_UNCOMPRESSIBLE;
#ifndef CO_AGGRESSIVE_CACHING
            const uint32_t name = compressor.getName();
            compressor.reset();
            LBCHECK( compressor.Compressor::initCompressor( name ));

            if( result == STATE_COMPLETE )
                buffer.pack();
#endif
            return;
        }

        state = result;
#ifndef CO_AGGRESSIVE_CACHING
        if( result == STATE_COMPLETE )
        {
            LBASSERT( buffer.getSize() == dataSize );
            buffer.clear();
        }
#endif
    }
};
}

DataOStream::DataOStream()
        : _impl( new detail::DataOStream )
{}

DataOStream::~DataOStream()
{
    // Can't call disable() from destructor since it uses virtual functions
    LBASSERT( !_impl->enabled );
    delete _impl;
}

void DataOStream::_initCompressor( const uint32_t compressor )
{
    LBCHECK( _impl->compressor.Compressor::initCompressor( compressor ));
    LB_TS_RESET( _impl->compressor._thread );
}

void DataOStream::_enable()
{
    LBASSERT( !_impl->enabled );
    LBASSERT( _impl->save || !_impl->connections.empty( ));
    _impl->state = STATE_UNCOMPRESSED;
    _impl->bufferStart = 0;
    _impl->dataSent    = false;
    _impl->dataSize    = 0;
    _impl->enabled     = true;
    _impl->buffer.setSize( 0 );
#ifdef CO_AGGRESSIVE_CACHING
    _impl->buffer.reserve( Buffer::getMinSize( ));
#endif
}

void DataOStream::_setupConnections( const Nodes& receivers )
{
    gatherConnections( receivers, _impl->connections );
}

void DataOStream::_setupConnections( const Connections& connections )
{
    _impl->connections = connections;
}

void DataOStream::_setupConnection( NodePtr node, const bool useMulticast )
{
    LBASSERT( _impl->connections.empty( ));
    ConnectionPtr connection = useMulticast ? node->useMulticast() : 0;
    if( !connection )
        connection = node->getConnection();

    _impl->connections.push_back( connection );
}

void DataOStream::_setupConnection( ConnectionPtr connection )
{
    _impl->connections.push_back( connection );
}

void DataOStream::_resend()
{
    LBASSERT( !_impl->enabled );
    LBASSERT( !_impl->connections.empty( ));
    LBASSERT( _impl->save );

    _impl->compress( _impl->buffer.getData(), _impl->dataSize, STATE_COMPLETE );
    sendData( _impl->buffer.getData(), _impl->dataSize, true );
}

void DataOStream::disable()
{
    if( !_disable( ))
        return;
    _impl->connections.clear();
}

void DataOStream::_clearConnections()
{
    _impl->connections.clear();
}

bool DataOStream::_disable()
{
    if( !_impl->enabled )
        return false;

    if( _impl->dataSent )
    {
        _impl->dataSize = _impl->buffer.getSize();
        if( !_impl->connections.empty( ))
        {
            void* ptr = _impl->buffer.getData() + _impl->bufferStart;
            const uint64_t size = _impl->buffer.getSize() - _impl->bufferStart;

            if( size == 0 && _impl->bufferStart == _impl->dataSize &&
                _impl->state == STATE_PARTIAL )
            {
                // OPT: all data has been sent in one compressed chunk
                _impl->state = STATE_COMPLETE;
#ifndef CO_AGGRESSIVE_CACHING
                _impl->buffer.clear();
#endif
            }
            else
            {
                _impl->state = STATE_UNCOMPRESSED;
                _impl->compress( ptr, size, STATE_PARTIAL );
            }

            sendData( ptr, size, true ); // always send to finalize istream
        }
    }
    else if( _impl->buffer.getSize() > 0 )
    {
        _impl->dataSize = _impl->buffer.getSize();
        _impl->dataSent = true;

        LBASSERT( _impl->bufferStart == 0 );
        if( !_impl->connections.empty( ))
        {
            _impl->state = STATE_UNCOMPRESSED;
            _impl->compress( _impl->buffer.getData(), _impl->dataSize,
                             STATE_COMPLETE );
            sendData( _impl->buffer.getData(), _impl->dataSize, true );
        }
    }

#ifndef CO_AGGRESSIVE_CACHING
    if( !_impl->save )
        _impl->buffer.clear();
#endif
    _impl->enabled = false;
    return true;
}

void DataOStream::enableSave()
{
    LBASSERTINFO( !_impl->enabled ||
                  ( !_impl->dataSent && _impl->buffer.getSize() == 0 ),
                  "Can't enable saving after data has been written" );
    _impl->save = true;
}

void DataOStream::disableSave()
{
    LBASSERTINFO( !_impl->enabled ||
                  (!_impl->dataSent && _impl->buffer.getSize() == 0 ),
                  "Can't disable saving after data has been written" );
    _impl->save = false;
}

bool DataOStream::hasSentData() const
{
    return _impl->dataSent;
}

void DataOStream::_write( const void* data, uint64_t size )
{
    LBASSERT( _impl->enabled );
#ifdef EQ_INSTRUMENT_DATAOSTREAM
    nBytes += size;
    if( compressionTime > 100000 )
        LBWARN << *this << std::endl;
#endif

    if( _impl->buffer.getSize() - _impl->bufferStart >
        Global::getObjectBufferSize( ))
    {
        _flush();
    }
    _impl->buffer.append( static_cast< const uint8_t* >( data ), size );
}

void DataOStream::_flush()
{
    LBASSERT( _impl->enabled );
    if( !_impl->connections.empty( ))
    {
        void* ptr = _impl->buffer.getData() + _impl->bufferStart;
        const uint64_t size = _impl->buffer.getSize() - _impl->bufferStart;

        _impl->state = STATE_UNCOMPRESSED;
        _impl->compress( ptr, size, STATE_PARTIAL );
        sendData( ptr, size, false );
    }
    _impl->dataSent = true;
    _resetBuffer();
}

void DataOStream::reset()
{
    _resetBuffer();
}

const Connections& DataOStream::getConnections() const
{
    return _impl->connections;
}

void DataOStream::_resetBuffer()
{
    _impl->state = STATE_UNCOMPRESSED;
    if( _impl->save )
        _impl->bufferStart = _impl->buffer.getSize();
    else
    {
        _impl->bufferStart = 0;
        _impl->buffer.setSize( 0 );
    }
}

uint64_t DataOStream::_getCompressedData( void** chunks, uint64_t* chunkSizes )
    const
{
    LBASSERT( _impl->state != STATE_UNCOMPRESSED &&
              _impl->state != STATE_UNCOMPRESSIBLE );

    const uint32_t nChunks = _impl->compressor.getNumResults( );
    LBASSERT( nChunks > 0 );

    uint64_t dataSize = 0;
    for ( uint32_t i = 0; i < nChunks; i++ )
    {
        _impl->compressor.getResult( i, &chunks[i], &chunkSizes[i] );
        dataSize += chunkSizes[i];
        LBASSERTINFO( chunkSizes[i] != 0, i );
    }

    return dataSize;
}

lunchbox::Bufferb& DataOStream::getBuffer()
{
    return _impl->buffer;
}

DataOStream& DataOStream::streamDataHeader( DataOStream& os )
{
    os << _impl->getCompressor() << _impl->getNumChunks();
    return os;
}

void DataOStream::sendData( ConnectionPtr connection, const uint64_t dataSize )
{
    const uint32_t compressor = _impl->getCompressor();
    if( compressor == EQ_COMPRESSOR_NONE )
    {
        if( dataSize > 0 )
            LBCHECK( connection->send( _impl->buffer.getData(), dataSize,
                                       true ));
        return;
    }

#ifdef EQ_INSTRUMENT_DATAOSTREAM
    nBytesSent += _impl->buffer.getSize();
#endif
    const uint32_t nChunks = _impl->compressor.getNumResults();
    uint64_t* chunkSizes =static_cast< uint64_t* >
                               ( alloca (nChunks * sizeof( uint64_t )));
    void** chunks = static_cast< void ** >
                                  ( alloca( nChunks * sizeof( void* )));

#ifdef EQ_INSTRUMENT_DATAOSTREAM
    const uint64_t compressedSize = _getCompressedData( chunks, chunkSizes );
    nBytesSaved += _impl->buffer.getSize() - compressedSize;
#else
    _getCompressedData( chunks, chunkSizes );
#endif

    for( size_t j = 0; j < nChunks; ++j )
    {
        LBCHECK( connection->send( &chunkSizes[j], sizeof( uint64_t ), true ));
        LBCHECK( connection->send( chunks[j], chunkSizes[j], true ));
    }
}

uint64_t DataOStream::getCompressedDataSize() const
{
    if( _impl->getCompressor() == EQ_COMPRESSOR_NONE )
        return 0;
    return _impl->compressedDataSize
            + _impl->getNumChunks() * sizeof( uint64_t );
}

std::ostream& operator << ( std::ostream& os, const DataOStream& dataOStream )
{
    os << "DataOStream "
#ifdef EQ_INSTRUMENT_DATAOSTREAM
       << "compressed " << nBytesIn << " -> " << nBytesOut << " of " << nBytes
       << " in " << compressionTime/1000 << "ms, saved " << nBytesSaved
       << " of " << nBytesSent << " brutto sent";

    nBytes = 0;
    nBytesIn = 0;
    nBytesOut = 0;
    nBytesSaved = 0;
    nBytesSent = 0;
    compressionTime = 0;
#else
       << "@" << (void*)&dataOStream;
#endif
    return os;
}

}
