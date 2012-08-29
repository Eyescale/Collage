
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

#include "objectDataOCommand.h"

#include "plugins/compressorTypes.h"

namespace co
{

namespace detail
{

class ObjectDataOCommand
{
public:
    ObjectDataOCommand( const void* buffer, const uint64_t dataSize_,
                        co::DataOStream* stream_ )
        : objectBuffer( buffer )
        , dataSize( dataSize_ )
        , userBuffer()
        , stream( stream_ )
    {}

    const void* objectBuffer;
    uint64_t dataSize;
    lunchbox::Bufferb userBuffer;
    co::DataOStream* stream;
};

}

ObjectDataOCommand::ObjectDataOCommand( const Connections& receivers,
                                        const uint32_t cmd, const uint32_t type,
                                        const UUID& id,
                                        const uint32_t instanceID,
                                        const uint128_t& version,
                                        const uint32_t sequence,
                                        const uint64_t dataSize,
                                        const bool isLast,
                                        const void* buffer,
                                        DataOStream* stream )
    : ObjectOCommand( receivers, cmd, type, id, instanceID )
    , _impl( new detail::ObjectDataOCommand( buffer, dataSize, stream ))
{
    const uint32_t compressor = stream ? stream->getCompressor()
                                       : EQ_COMPRESSOR_NONE;
    const uint32_t nChunks = stream ? stream->getNumChunks() : 1;

    if( compressor != EQ_COMPRESSOR_NONE )
        _impl->dataSize = stream->getCompressedDataSize() +
                          nChunks * sizeof( uint64_t );

    // cast to avoid call to our user data operator << overload
    (ObjectOCommand&)(*this) << version << sequence << dataSize << isLast
                             << compressor << nChunks;
}

ObjectDataOCommand::~ObjectDataOCommand()
{
    disable();
    delete _impl;
}

void ObjectDataOCommand::_addUserData( const void* data, uint64_t size )
{
    _impl->userBuffer.append( static_cast< const uint8_t* >( data ), size );
}

void ObjectDataOCommand::sendData( const void* buffer, const uint64_t size,
                                   const bool last )
{
    LBASSERT( last );
    LBASSERT( size > 0 );

    const uint64_t finalSize = size +                        // packet header
                               _impl->userBuffer.getSize() + // userBuffer
                               _impl->dataSize;              // objectBuffer

    for( ConnectionsCIter it = getConnections().begin();
         it != getConnections().end(); ++it )
    {
        ConnectionPtr conn = *it;

        conn->lockSend();
        conn->send( &finalSize, sizeof( uint64_t ), true );
        conn->send( buffer, size, true );
        if( !_impl->userBuffer.isEmpty( ))
            conn->send( _impl->userBuffer.getData(),
                        _impl->userBuffer.getSize(), true );
        if( !_impl->stream || _impl->dataSize == 0 )
        {
            conn->unlockSend();
            continue;
        }

        if( _impl->stream->getCompressor() == EQ_COMPRESSOR_NONE )
            conn->send( _impl->objectBuffer, _impl->dataSize, true );
        else
            _impl->stream->sendCompressedData( conn );

        conn->unlockSend();
    }
}

}
