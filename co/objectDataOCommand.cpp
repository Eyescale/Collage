
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
    ObjectDataOCommand( const uint128_t& version_, const uint32_t sequence_,
                        const uint64_t dataSize_, const bool isLast_,
                        co::DataOStream* stream_ )
        : version( version_ )
        , sequence( sequence_ )
        , isLast( isLast_ )
        , dataSize( dataSize_ )
        , userBuffer()
        , stream( stream_ )
    {
        if( stream )
        {
            dataSize = stream->getCompressedDataSize();
            if( dataSize == 0 )
                dataSize = dataSize_;
        }
    }
    
    ObjectDataOCommand( ObjectDataOCommand& rhs )
        : version( rhs.version )
        , sequence( rhs.sequence )
        , isLast( rhs.isLast )
        , dataSize( rhs.dataSize )
        , userBuffer()
        , stream( rhs.stream )
    {
        userBuffer.swap( rhs.userBuffer );
    }

    const uint128_t& version;
    const uint32_t sequence;
    const bool isLast;
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
                                        DataOStream* stream )
    : ObjectOCommand( receivers, cmd, type, id, instanceID )
    , _impl( new detail::ObjectDataOCommand( version, sequence, dataSize, isLast,
                                             stream ))
{
    _init();
}

ObjectDataOCommand::ObjectDataOCommand( const ObjectDataOCommand& rhs )
    : ObjectOCommand( rhs )
    , _impl( new detail::ObjectDataOCommand( *rhs._impl ))
{
    _init();
}

void ObjectDataOCommand::_init()
{
    // cast to avoid call to our user data operator << overload
    (ObjectOCommand&)(*this) << _impl->version << _impl->sequence
                             << _impl->dataSize << _impl->isLast;

    if( _impl->stream )
        _impl->stream->streamDataHeader( *this );
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

    const uint64_t finalSize = size +                        // command header
                               _impl->userBuffer.getSize() + // userBuffer
                               _impl->dataSize;

    for( ConnectionsCIter it = getConnections().begin();
         it != getConnections().end(); ++it )
    {
        ConnectionPtr conn = *it;

        conn->lockSend();
        LBCHECK( conn->send( &finalSize, sizeof( uint64_t ), true ));
        LBCHECK( conn->send( buffer, size, true ));
        if( !_impl->userBuffer.isEmpty( ))
            LBCHECK( conn->send( _impl->userBuffer.getData(),
                                 _impl->userBuffer.getSize(), true ));
        if( _impl->stream )
            _impl->stream->sendData( conn, _impl->dataSize );

        conn->unlockSend();
    }
}

}
