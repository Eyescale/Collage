
/* Copyright (c) 2007-2016, Stefan Eilemann <eile@equalizergraphics.com>
 *                          Cedric Stalder <cedric.stalder@gmail.com>
 *                          Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "dataOStream.h"

#include "buffer.h"
#include "connectionDescription.h"
#include "commands.h"
#include "connections.h"
#include "global.h"
#include "log.h"
#include "node.h"
#include "types.h"

#include <lunchbox/clock.h>
#include <pression/data/Compressor.h>
#include <pression/data/CompressorInfo.h>

namespace co
{
namespace
{
#define CO_INSTRUMENT_DATAOSTREAM
#ifdef CO_INSTRUMENT_DATAOSTREAM
lunchbox::a_ssize_t nBytes;
lunchbox::a_ssize_t nBytesIn;
lunchbox::a_ssize_t nBytesOut;
lunchbox::a_ssize_t nBytesSaved;
lunchbox::a_ssize_t nBytesSent;
lunchbox::a_ssize_t compressionTime;
lunchbox::a_ssize_t compressionRuns;
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

    CompressorState state; //!< State of compression
    CompressorPtr compressor; //!< The compressor instance.
    CompressorInfo compressorInfo; //!< Information on the compr.

    /** The output stream is enabled for writing */
    bool enabled;

    /** Some data has been sent since it was enabled */
    bool dataSent;

    /** Save all sent data */
    bool save;

    DataOStream()
        : bufferStart( 0 )
        , dataSize( 0 )
        , compressedDataSize( 0 )
        , state( STATE_UNCOMPRESSED )
        , enabled( false )
        , dataSent( false )
        , save( false )
    {}

    DataOStream( const DataOStream& rhs )
        : bufferStart( rhs.bufferStart )
        , dataSize( rhs.dataSize )
        , compressedDataSize( rhs.compressedDataSize )
        , state( rhs.state )
        , enabled( rhs.enabled )
        , dataSent( rhs.dataSent )
        , save( rhs.save )
    {}

    std::string getCompressorName() const
    {
        if( state == STATE_UNCOMPRESSED || state == STATE_UNCOMPRESSIBLE ||
            !compressor )
        {
            return std::string();
        }
        return compressorInfo.name;
    }

    bool initCompressor()
    {
        if( compressorInfo.name.empty( ))
            return false;
        if( compressor )
            return true;

        compressor.reset( compressorInfo.create( ));
        LBLOG( LOG_OBJECTS ) << "Allocated " << compressorInfo.name <<std::endl;
        return compressor != nullptr;
    }

    uint32_t getNumChunks() const
    {
        if( state == STATE_UNCOMPRESSED || state == STATE_UNCOMPRESSIBLE ||
            !compressor )
        {
            return 1;
        }
        return compressor->getCompressedData().size();
    }

    /** Compress data and update the compressor state. */
    void compress( const uint8_t* src, const uint64_t size,
                   const CompressorState result )
    {
        if( state == result || state == STATE_UNCOMPRESSIBLE )
            return;
        const uint64_t threshold =
           uint64_t( Global::getIAttribute( Global::IATTR_OBJECT_COMPRESSION ));

        if( size <= threshold || !initCompressor( ))
        {
            state = STATE_UNCOMPRESSED;
            return;
        }

#ifdef CO_INSTRUMENT_DATAOSTREAM
        lunchbox::Clock clock;
#endif
        const auto& output = compressor->compress( src, size );
        LBASSERT( !output.empty( ));
        compressedDataSize = pression::data::getDataSize( output );

#ifdef CO_INSTRUMENT_DATAOSTREAM
        compressionTime += size_t( clock.getTimef() * 1000.f );
        nBytesIn += size;
        nBytesOut += compressedDataSize;
        ++compressionRuns;
#endif

        if( compressedDataSize >= size )
        {
            state = STATE_UNCOMPRESSIBLE;
#ifndef CO_AGGRESSIVE_CACHING
            compressor.reset( compressorInfo.create( ));

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

DataOStream::DataOStream( DataOStream& rhs )
    : boost::noncopyable()
    , _impl( new detail::DataOStream( *rhs._impl ))
{
    _setupConnections( rhs.getConnections( ));
    getBuffer().swap( rhs.getBuffer( ));

    // disable send of rhs
    rhs._setupConnections( Connections( ));
    rhs.disable();
}

DataOStream::~DataOStream()
{
    // Can't call disable() from destructor since it uses virtual functions
    LBASSERT( !_impl->enabled );
    delete _impl;
}

void DataOStream::_setCompressor( const CompressorInfo& info )
{
    if( info == _impl->compressorInfo )
        return;
    _impl->compressorInfo = info;
    _impl->compressor.reset( nullptr );
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
    _impl->buffer.reserve( COMMAND_ALLOCSIZE );
#else
    _impl->buffer.reserve( COMMAND_MINSIZE );
#endif
}

void DataOStream::_setupConnections( const Nodes& receivers )
{
    _impl->connections = gatherConnections( receivers );
}

void DataOStream::_setupConnections( const Connections& connections )
{
    _impl->connections = connections;
}

void DataOStream::_setupConnection( NodePtr node, const bool useMulticast )
{
    LBASSERT( _impl->connections.empty( ));
    _impl->connections.push_back( node->getConnection( useMulticast ));
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

void DataOStream::_clearConnections()
{
    _impl->connections.clear();
}

void DataOStream::disable()
{
    if( !_impl->enabled )
        return;

    _impl->dataSize = _impl->buffer.getSize();
    _impl->dataSent = _impl->dataSize > 0;

    if( _impl->dataSent && !_impl->connections.empty( ))
    {
        const uint8_t* ptr = _impl->buffer.getData() + _impl->bufferStart;
        const uint64_t size = _impl->buffer.getSize() - _impl->bufferStart;

        if( size == 0 && _impl->state == STATE_PARTIAL )
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
            const CompressorState state = _impl->bufferStart == 0 ?
                                              STATE_COMPLETE : STATE_PARTIAL;
            _impl->compress( ptr, size, state );
        }

        sendData( ptr, size, true ); // always send to finalize istream
    }

#ifndef CO_AGGRESSIVE_CACHING
    if( !_impl->save )
        _impl->buffer.clear();
#endif
    _impl->enabled = false;
    _impl->connections.clear();
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
#ifdef CO_INSTRUMENT_DATAOSTREAM
    nBytes += size;
#endif

    if( _impl->buffer.getSize() - _impl->bufferStart >
        Global::getObjectBufferSize( ))
    {
        flush( false );
    }
    _impl->buffer.append( static_cast< const uint8_t* >( data ), size );
}

void DataOStream::flush( const bool last )
{
    LBASSERT( _impl->enabled );
    if( !_impl->connections.empty( ))
    {
        const uint8_t* ptr = _impl->buffer.getData() + _impl->bufferStart;
        const uint64_t size = _impl->buffer.getSize() - _impl->bufferStart;

        _impl->state = STATE_UNCOMPRESSED;
        _impl->compress( ptr, size, STATE_PARTIAL );
        sendData( ptr, size, last );
    }
    _impl->dataSent = true;
    _resetBuffer();
}

void DataOStream::reset()
{
    _resetBuffer();
    _impl->enabled = false;
    _impl->connections.clear();
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

uint64_t DataOStream::_getCompressedData( const uint8_t** chunks,
                                          uint64_t* chunkSizes )
    const
{
    LBASSERT( _impl->state != STATE_UNCOMPRESSED &&
              _impl->state != STATE_UNCOMPRESSIBLE && _impl->compressor );

    const auto& result = _impl->compressor->getCompressedData();
    LBASSERT( !result.empty() );
    size_t totalDataSize = 0;
    for( size_t i = 0; i != result.size(); ++i )
    {
        chunks[i] = result[i].getData();
        const size_t dataSize = result[i].getSize();
        chunkSizes[i] = dataSize;
        totalDataSize += dataSize;
    }

    LBASSERT( totalDataSize == pression::data::getDataSize( result ));
    return totalDataSize;
}

lunchbox::Bufferb& DataOStream::getBuffer()
{
    return _impl->buffer;
}

DataOStream& DataOStream::streamDataHeader( DataOStream& os )
{
    return os << _impl->getCompressorName() << _impl->getNumChunks();
}

void DataOStream::sendBody( ConnectionPtr connection, const void* data,
                            const uint64_t size )
{

    const auto& compressorName = _impl->getCompressorName();
    if( compressorName.empty( ))
    {
#ifdef CO_INSTRUMENT_DATAOSTREAM
        nBytesSent += size;
#endif
        if( size > 0 )
            LBCHECK( connection->send( data, size, true ));
        return;
    }

    const size_t nChunks = _impl->compressor->getCompressedData().size();
    uint64_t* chunkSizes = static_cast< uint64_t* >
                               ( alloca (nChunks * sizeof( uint64_t )));
    const uint8_t** chunks = static_cast< const uint8_t ** >
                                  ( alloca( nChunks * sizeof( void* )));

#ifdef CO_INSTRUMENT_DATAOSTREAM
    const uint64_t compressedSize = _getCompressedData( chunks, chunkSizes );
    if( _impl->state == STATE_COMPLETE )
    {
        nBytesSent += _impl->dataSize;
        nBytesSaved += _impl->dataSize - compressedSize;
    }
    else
    {
        nBytesSent += _impl->buffer.getSize() - _impl->bufferStart;
        nBytesSaved += _impl->buffer.getSize() - _impl->bufferStart -
                       compressedSize;
    }

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
    if( _impl->state == STATE_UNCOMPRESSED ||
        _impl->state == STATE_UNCOMPRESSIBLE || !_impl->compressor )
    {
        return 0;
    }
    return _impl->compressedDataSize +
           _impl->getNumChunks() * sizeof( uint64_t );
}

std::ostream& DataOStream::printStatistics( std::ostream& os )
{
    return os << "DataOStream "
#ifdef CO_INSTRUMENT_DATAOSTREAM
              << "compressed " << nBytesIn << " -> " << nBytesOut << " of "
              << nBytes << " @ "
              << int( float( nBytesIn )/1.024f/1.024f/compressionTime + .5f )
              << " MB/s " << compressionRuns << " runs, saved " << nBytesSaved
              << " of " << nBytesSent << " brutto sent ("
              << double( nBytesSaved ) / double( nBytesSent ) * 100. << "%)";
#else
              << "without statistics enabled";
#endif
}

void DataOStream::clearStatistics()
{
#ifdef CO_INSTRUMENT_DATAOSTREAM
    nBytes = 0;
    nBytesIn = 0;
    nBytesOut = 0;
    nBytesSaved = 0;
    nBytesSent = 0;
    compressionTime = 0;
    compressionRuns = 0;
#endif
}

}
