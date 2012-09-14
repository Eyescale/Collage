
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
    ObjectDataOCommand( co::DataOStream* stream_, const uint64_t dataSize_ )
        : dataSize( 0 )
        , stream( stream_ )
    {
        if( stream )
        {
            dataSize = stream->getCompressedDataSize();
            if( dataSize == 0 )
                dataSize = dataSize_;
        }
    }

    ObjectDataOCommand( const ObjectDataOCommand& rhs )
        : dataSize( rhs.dataSize )
        , stream( rhs.stream )
    {}

    uint64_t dataSize;
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
    , _impl( new detail::ObjectDataOCommand( stream, dataSize ))
{
    _init( version, sequence, dataSize, isLast );
}

ObjectDataOCommand::ObjectDataOCommand( const ObjectDataOCommand& rhs )
    : ObjectOCommand( rhs )
    , _impl( new detail::ObjectDataOCommand( *rhs._impl ))
{
}

void ObjectDataOCommand::_init( const uint128_t& version,
                                const uint32_t sequence,
                                const uint64_t dataSize, const bool isLast )
{
    *this << version << sequence << dataSize << isLast;

    if( _impl->stream )
        _impl->stream->streamDataHeader( *this );
}

ObjectDataOCommand::~ObjectDataOCommand()
{
    if( _impl->stream )
    {
        sendHeader( _impl->dataSize );
        const Connections& connections = getConnections();
        for( ConnectionsCIter i = connections.begin(); i != connections.end();
             ++i )
        {
            ConnectionPtr conn = *i;
            _impl->stream->sendData( conn, _impl->dataSize );
        }
    }

    delete _impl;
}

}
