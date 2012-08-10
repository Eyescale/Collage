
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

#include "nodeOCommand.h"


namespace co
{

namespace detail
{

class NodeOCommand
{
public:
    NodeOCommand( ConnectionPtr connection_, uint32_t type_, uint32_t cmd_ )
        : connection( connection_ )
        , type( type_ )
        , cmd( cmd_ )
    {}

    ConnectionPtr connection;
    uint32_t type;
    uint32_t cmd;
};

}

NodeOCommand::NodeOCommand( ConnectionPtr connection, uint32_t type,
                            uint32_t cmd )
    : DataOStream()
    , _impl( new detail::NodeOCommand( connection, type, cmd ))
{
    _setupConnection( _impl->connection );
    enableSave();
    _enable();
    *this << type << cmd;
}

NodeOCommand::~NodeOCommand()
{
    disable();
    delete _impl;
}

void NodeOCommand::sendData( const void* buffer, const uint64_t size,
                             const bool last )
{
    LBASSERT( last );
    LBASSERT( size > 0 );

    _impl->connection->lockSend();
    // TEMP: signal new datastream 'packet' on the other side
    uint64_t magicSize = 0;
    _impl->connection->send( &magicSize, sizeof( uint64_t ), true );

    _impl->connection->send( &size, sizeof( uint64_t ), true );
    _impl->connection->send( buffer, size, true );
    _impl->connection->unlockSend();
}

}
