
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
#include "iCommand.h"

namespace co
{
namespace detail
{

class OCommand
{
public:
    OCommand( co::Dispatcher* const dispatcher_, LocalNodePtr localNode_ )
        : isLocked( false )
        , size( 0 )
        , dispatcher( dispatcher_ )
        , localNode( localNode_ )
    {}

    bool isLocked;
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
    if( _impl->isLocked )
    {
        LBASSERT( _impl->size > 0 );
        const uint64_t size = _impl->size + getBuffer().getSize();
        const size_t minSize = Buffer::getMinSize();
        const Connections& connections = getConnections();
        if( size < minSize ) // Fill send to minimal size
        {
            const size_t delta = minSize - size;
            void* padding = alloca( delta );
            for( ConnectionsCIter i = connections.begin();
                 i != connections.end(); ++i )
            {
                ConnectionPtr connection = *i;
                connection->send( padding, delta, true );
            }
        }
        for( ConnectionsCIter i = connections.begin();
             i != connections.end(); ++i )
        {
            ConnectionPtr connection = *i;
            connection->unlockSend();
        }
        _impl->isLocked = false;
        _impl->size = 0;
        reset();
    }
    else
        disable();

    if( _impl->dispatcher )
    {
        LBASSERT( _impl->localNode );

        // #145 proper local command dispatch?
        LBASSERT( _impl->size == 0 );
        const uint64_t size = getBuffer().getSize();
        BufferPtr buffer = _impl->localNode->allocBuffer( size );
        buffer->swap( getBuffer( ));
        reinterpret_cast< uint64_t* >( buffer->getData( ))[ 0 ] = size;

        ICommand cmd( _impl->localNode, _impl->localNode, buffer, false );
        _impl->dispatcher->dispatchCommand( cmd );
    }

    delete _impl;
}

void OCommand::sendHeader( const uint64_t additionalSize )
{
    LBASSERT( !_impl->dispatcher );
    LBASSERT( !_impl->isLocked );
    LBASSERT( additionalSize > 0 );

    const Connections& connections = getConnections();
    for( ConnectionsCIter i = connections.begin(); i != connections.end(); ++i )
    {
        ConnectionPtr connection = *i;
        connection->lockSend();
    }
    _impl->isLocked = true;
    _impl->size = additionalSize;
    flush( true );
}

size_t OCommand::getSize()
{
    return sizeof( uint64_t ) + sizeof( uint32_t ) + sizeof( uint32_t );
}

void OCommand::_init( const uint32_t cmd, const uint32_t type )
{
    LBASSERT( cmd < CMD_NODE_MAXIMUM );
    enableSave();
    _enable();
    *this << 0ull /* size */ << type << cmd;
}

void OCommand::sendData( const void* buffer, const uint64_t size,
                         const bool last )
{
    LBASSERT( !_impl->dispatcher );
    LBASSERT( last );
    LBASSERTINFO( size >= 16, size );
    LBASSERT( getBuffer().getData() == buffer );
    LBASSERT( getBuffer().getSize() == size );
    LBASSERT( getBuffer().getMaxSize() >= Buffer::getMinSize( ));

    // Update size field
    uint8_t* bytes = getBuffer().getData();
    reinterpret_cast< uint64_t* >( bytes )[ 0 ] = _impl->size + size;
    const uint64_t sendSize = _impl->isLocked ?
        size : LB_MAX( size, Buffer::getMinSize( ));

    const Connections& connections = getConnections();
    for( ConnectionsCIter i = connections.begin(); i != connections.end(); ++i )
    {
        ConnectionPtr connection = *i;
        connection->send( bytes, sendSize, _impl->isLocked );
    }
}

}
