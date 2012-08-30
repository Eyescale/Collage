
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
    NodeOCommand( const uint32_t cmd_, const uint32_t type_ )
        : type( type_ )
        , cmd( cmd_ )
        , lockSend( true )
        , size( 0 )
    {}

    // #145 eile
    // Shouldn't this have move semantics? Otherwise creating a copy will send
    // two packets? Alternatively disable copy ctor and return auto_ptr?
    NodeOCommand( const NodeOCommand& rhs )
        : type( rhs.type )
        , cmd( rhs.cmd )
        , lockSend( rhs.lockSend )
        , size( rhs.size )
    {}

    uint32_t type;
    uint32_t cmd;
    bool lockSend;
    uint64_t size;
};

}

NodeOCommand::NodeOCommand( const Connections& connections, const uint32_t cmd,
                            const uint32_t type )
    : DataOStream()
    , _impl( new detail::NodeOCommand( cmd, type ))
{
    _setupConnections( connections );
    _init();
}

NodeOCommand::NodeOCommand( NodeOCommand const& rhs )
    : DataOStream()
    , _impl( new detail::NodeOCommand( *rhs._impl ))
{
    _setupConnections( rhs.getConnections( ));
    _init();
}

NodeOCommand::~NodeOCommand()
{
    disable();
    delete _impl;
}

void NodeOCommand::sendUnlocked( const uint64_t additionalSize )
{
    _impl->lockSend = false;
    _impl->size = additionalSize;
    disable();
}

size_t NodeOCommand::getSize()
{
    return sizeof( uint32_t ) + sizeof( uint32_t );
}

void NodeOCommand::_init()
{
    enableSave();
    _enable();
    *this << _impl->type << _impl->cmd;
}

void NodeOCommand::sendData( const void* buffer, const uint64_t size,
                             const bool last )
{
    LBASSERT( last );
    LBASSERT( size > 0 );

    _impl->size += size;

    for( ConnectionsCIter it = getConnections().begin();
         it != getConnections().end(); ++it )
    {
        ConnectionPtr conn = *it;

        if( _impl->lockSend )
            conn->lockSend();
        conn->send( &_impl->size, sizeof( uint64_t ), true );
        conn->send( buffer, size, true );
        if( _impl->lockSend )
            conn->unlockSend();
    }

    _impl->lockSend = true;
    _impl->size = 0;
}

}
