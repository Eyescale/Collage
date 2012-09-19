
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
    {}

    Command( ConstBufferPtr buffer_ )
        : func( 0, 0 )
        , buffer( buffer_ )
        , type( COMMANDTYPE_INVALID )
        , cmd( CMD_INVALID )
    {}

    Command( const Command& rhs )
        : func( rhs.func )
        , buffer( rhs.buffer )
        , type( rhs.type )
        , cmd( rhs.cmd )
    {}

    void operator = ( const Command& rhs )
    {
        func = rhs.func;
        buffer = rhs.buffer;
        type = rhs.type;
        cmd = rhs.cmd;
    }

    void clear()
    {
        func.clear();
        buffer = 0;
        type = COMMANDTYPE_INVALID;
        cmd = CMD_INVALID;
    }

    co::Dispatcher::Func func;
    ConstBufferPtr buffer;
    uint32_t type;
    uint32_t cmd;
};
}

Command::Command()
    : DataIStream()
    , _impl( new detail::Command )
{
}

Command::Command( ConstBufferPtr buffer )
    : DataIStream()
    , _impl( new detail::Command( buffer ))
{
    if( _impl->buffer )
        *this >> _impl->type >> _impl->cmd;
}

Command::Command( const Command& rhs )
    : DataIStream()
    , _impl( new detail::Command( *rhs._impl ))
{
    setSwapping( rhs.isSwapping( )); // needed for node handshake commands
    if( _impl->buffer )
        _skipHeader();
}

Command& Command::operator = ( const Command& rhs )
{
    if( this != &rhs )
    {
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

void Command::reread()
{
    LBASSERT( _impl->buffer );
    if( !_impl->buffer )
        return;

    getRemainingBuffer( getRemainingBufferSize( )); // yuck. resets packet.
    *this >> _impl->type >> _impl->cmd;
}

void Command::_skipHeader()
{
    const size_t headerSize = sizeof( _impl->type ) + sizeof( _impl->cmd );
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

uint64_t Command::getSize() const
{
    LBASSERT( isValid( ));
    return _impl->buffer->getSize();
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

    if( _impl->buffer->getNode( ))
        setSwapping( _impl->buffer->needsSwapping( ));
    // else handshake command. see copy ctor and LocalNode::_dispatchCommand()
    return true;
}

NodePtr Command::getNode() const
{
    return _impl->buffer->getNode();
}

LocalNodePtr Command::getLocalNode() const
{
    return _impl->buffer->getLocalNode();
}

bool Command::isValid() const
{
    return _impl->buffer && _impl->buffer->isValid() &&
           _impl->type != COMMANDTYPE_INVALID && _impl->cmd != CMD_INVALID;
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
    if( command.isValid( ))
    {
        os << lunchbox::disableFlush << "command< ";
        os << " type " << uint32_t( command.getType( ))
           << " cmd " << command.getCommand();
        os << ", " << command.getNode() << " >" << lunchbox::enableFlush;
    }
    else
        os << "command< empty >";

    if( command._impl->func.isValid( ))
        os << ' ' << command._impl->func << std::endl;
    return os;
}
}
