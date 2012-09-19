
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "command.h"

#include "buffer.h"
#include "localNode.h"
#include "node.h"
#include "node.h"
#include "plugins/compressorTypes.h"

namespace co
{
namespace detail
{
class Command
{
public:
    Command()
        : func( 0, 0 )
        , buffer( 0 )
        , type( COMMANDTYPE_INVALID )
        , cmd( CMD_INVALID )
        , size( 0 )
    {}

    Command( LocalNodePtr local_, NodePtr remote_, ConstBufferPtr buffer_ )
        : local( local_ )
        , remote( remote_ )
        , func( 0, 0 )
        , buffer( buffer_ )
        , type( COMMANDTYPE_INVALID )
        , cmd( CMD_INVALID )
        , size( 0 )
    {}

    void clear()
    {
        *this = Command();
    }

    LocalNodePtr local; //!< The node receiving the command
    NodePtr remote; //!< The node sending the command
    co::Dispatcher::Func func;
    ConstBufferPtr buffer;
    uint32_t type;
    uint32_t cmd;
    uint64_t size;
};
}

Command::Command()
    : DataIStream( false )
    , _impl( new detail::Command )
{
}

Command::Command( LocalNodePtr local, NodePtr remote, ConstBufferPtr buffer,
                  const bool swap_ )
    : DataIStream( swap_ )
    , _impl( new detail::Command( local, remote, buffer ))
{
    if( _impl->buffer )
        *this >> _impl->size >> _impl->type >> _impl->cmd;
}

Command::Command( const Command& rhs )
    : DataIStream( rhs )
    , _impl( new detail::Command( *rhs._impl ))
{
    _skipHeader();
}

Command& Command::operator = ( const Command& rhs )
{
    if( this != &rhs )
    {
        DataIStream::operator = ( rhs );
        *_impl = *rhs._impl;
        _skipHeader();
    }
    return *this;
}

Command::~Command()
{
    delete _impl;
}

void Command::clear()
{
    _impl->clear();
}

void Command::_skipHeader()
{
    const size_t headerSize = sizeof( _impl->size ) + sizeof( _impl->type ) +
                              sizeof( _impl->cmd );
    if( isValid() && getRemainingBufferSize() >= headerSize )
        getRemainingBuffer( headerSize );
}

uint32_t Command::getType() const
{
    return _impl->type;
}

uint32_t Command::getCommand() const
{
    return _impl->cmd;
}

uint64_t Command::getSize_() const
{
    return _impl->size;
}

void Command::setType( const CommandType type )
{
    _impl->type = type;
}

void Command::setCommand( const uint32_t cmd )
{
    _impl->cmd = cmd;
}

void Command::setDispatchFunction( const Dispatcher::Func& func )
{
    _impl->func = func;
}

ConstBufferPtr Command::getBuffer() const
{
    LBASSERT( _impl->buffer );
    return _impl->buffer;
}

size_t Command::nRemainingBuffers() const
{
    return _impl->buffer ? 1 : 0;
}

uint128_t Command::getVersion() const
{
    return VERSION_NONE;
}

NodePtr Command::getMaster()
{
    return getNode();
}

bool Command::getNextBuffer( uint32_t& compressor, uint32_t& nChunks,
                             const void** chunkData, uint64_t& size )
{
    if( !_impl->buffer )
        return false;

    *chunkData = _impl->buffer->getData();
    size = _impl->buffer->getSize();
    compressor = EQ_COMPRESSOR_NONE;
    nChunks = 1;

    return true;
}

NodePtr Command::getNode() const
{
    return _impl->remote;
}

LocalNodePtr Command::getLocalNode() const
{
    return _impl->local;
}

bool Command::isValid() const
{
    return _impl->buffer && !_impl->buffer->isEmpty() &&
           _impl->type != COMMANDTYPE_INVALID && _impl->cmd != CMD_INVALID &&
           _impl->size > 0;
}

bool Command::operator()()
{
    LBASSERT( _impl->func.isValid( ));
    Dispatcher::Func func = _impl->func;
    _impl->func.clear();
    return func( *this );
}

std::ostream& operator << ( std::ostream& os, const Command& command )
{
    ConstBufferPtr buffer = command.getBuffer();
    if( buffer )
        os << lunchbox::disableFlush << "command< type "
           << uint32_t( command.getType( )) << " cmd " << command.getCommand()
           << " size " << command.getSize_() << '/' << buffer->getSize() << '/'
           << buffer->getMaxSize() << " from " << command.getNode() << " to "
           << command.getLocalNode() << " >" << lunchbox::enableFlush;
    else
        os << "command< empty >";

    if( command._impl->func.isValid( ))
        os << ' ' << command._impl->func << std::endl;
    return os;
}
}
