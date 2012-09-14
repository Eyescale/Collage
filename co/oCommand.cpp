
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

#include "oCommand.h"

#include "buffer.h"
#include "command.h"


namespace co
{

namespace detail
{

class OCommand
{
public:
    OCommand( co::Dispatcher* const dispatcher_, LocalNodePtr localNode_ )
        : lockSend( true )
        , size( 0 )
        , dispatcher( dispatcher_ )
        , localNode( localNode_ )
    {
    }

    OCommand( const OCommand& rhs )
        : lockSend( rhs.lockSend )
        , size( rhs.size )
        , dispatcher( rhs.dispatcher )
        , localNode( rhs.localNode )
    {}

    bool lockSend;
    uint64_t size;
    co::Dispatcher* const dispatcher;
    LocalNodePtr localNode;
};

}

OCommand::OCommand( const Connections& receivers, const uint32_t cmd,
                    const uint32_t type )
    : DataOStream()
    , _impl( new detail::OCommand( 0, 0 ))
{
    _setupConnections( receivers );
    _init( cmd, type );
}

OCommand::OCommand( Dispatcher* const dispatcher, LocalNodePtr localNode,
                    const uint32_t cmd, const uint32_t type )
    : DataOStream()
    , _impl( new detail::OCommand( dispatcher, localNode ))
{
    _init( cmd, type );
}

OCommand::OCommand( const OCommand& rhs )
    : DataOStream()
    , _impl( new detail::OCommand( *rhs._impl ))
{
    _setupConnections( rhs.getConnections( ));
    getBuffer().swap( const_cast< OCommand& >( rhs ).getBuffer( ));

    // disable send of rhs
    const_cast< OCommand& >( rhs )._setupConnections( Connections( ));
    const_cast< OCommand& >( rhs ).disable();
}

OCommand::~OCommand()
{
    disable();

    if( _impl->dispatcher )
    {
        LBASSERT( _impl->localNode );

        // #145 proper local command dispatch?
        BufferPtr buffer =
                _impl->localNode->allocCommand( getBuffer().getSize( ));
        buffer->swap( getBuffer( ));
        Command cmd( buffer );
        _impl->dispatcher->dispatchCommand( cmd );
    }

    delete _impl;
}

void OCommand::sendHeaderUnlocked( const uint64_t additionalSize )
{
    LBASSERT( !_impl->dispatcher );
    _impl->lockSend = false;
    _impl->size = additionalSize;
    disable();
}

size_t OCommand::getSize()
{
    return sizeof( uint32_t ) + sizeof( uint32_t );
}

void OCommand::_init( const uint32_t cmd, const uint32_t type )
{
    LBASSERT( cmd < CMD_NODE_MAXIMUM );
    enableSave();
    _enable();
    *this << type << cmd;
}

void OCommand::sendData( const void* buffer, const uint64_t size,
                         const bool last )
{
    LBASSERT( !_impl->dispatcher );
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
