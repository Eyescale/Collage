
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

#include "nodeDataOStream.h"


namespace co
{

namespace detail
{

class NodeDataOStream
{
public:
    NodeDataOStream( ConnectionPtr connection_, uint32_t type_, uint32_t cmd_ )
        : connection( connection_ )
        , type( type_ )
        , cmd( cmd_ )
    {}

    NodeDataOStream( const NodeDataOStream& rhs )
        : connection( rhs.connection )
        , type( rhs.type )
        , cmd( rhs.cmd )
    {}

    ConnectionPtr connection;
    uint32_t type;
    uint32_t cmd;
};

}

NodeDataOStream::NodeDataOStream( ConnectionPtr connection, uint32_t type,
                                  uint32_t cmd )
    : DataOStream()
    , _impl( new detail::NodeDataOStream( connection, type, cmd ))
{
    _setupConnection( _impl->connection );
    enableSave();
    _enable();
}

NodeDataOStream::NodeDataOStream( NodeDataOStream const& rhs )
    : DataOStream()
    , _impl( new detail::NodeDataOStream( *rhs._impl ))
{
    _setupConnection( _impl->connection );
    enableSave();
    _enable();
}

NodeDataOStream::~NodeDataOStream()
{
    disable();
    delete _impl;
}

void NodeDataOStream::sendData( const void* buffer, const uint64_t size,
                                const bool last )
{
    LBASSERT( last );
    LBASSERT( size > 0 );

    _impl->connection->lockSend();
    // TEMP: signal new datastream 'packet' on the other side
    uint64_t magicSize = 0;
    _impl->connection->send( &magicSize, sizeof( uint64_t ), true );

    // include type & cmd in payload size
    uint64_t actualSize = size + sizeof( uint32_t ) + sizeof( uint32_t );
    _impl->connection->send( &actualSize, sizeof( uint64_t ), true );
    _impl->connection->send( &_impl->type, sizeof( uint32_t ), true );
    _impl->connection->send( &_impl->cmd, sizeof( uint32_t ), true );
    _impl->connection->send( buffer, size, true );
    _impl->connection->unlockSend();
}

}
