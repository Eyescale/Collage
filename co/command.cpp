
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
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
        : _func( 0, 0 )
        , _buffer( 0 )
        , _type( COMMANDTYPE_INVALID )
        , _cmd( CMD_INVALID )
    {}

    Command( BufferPtr buffer )
        : _func( 0, 0 )
        , _buffer( buffer )
        , _type( COMMANDTYPE_INVALID )
        , _cmd( CMD_INVALID )
    {}

    Command( const Command& rhs )
        : _func( rhs._func )
        , _buffer( rhs._buffer )
        , _type( rhs._type )
        , _cmd( rhs._cmd )
    {}

    void operator=( const Command& rhs )
    {
        _func = rhs._func;
        _buffer = rhs._buffer;
        _type = rhs._type;
        _cmd = rhs._cmd;
    }

    co::Dispatcher::Func _func;
    BufferPtr _buffer;
    uint32_t _type;
    uint32_t _cmd;
};
}

Command::Command()
    : DataIStream()
    , _impl( new detail::Command )
{
}

Command::Command( BufferPtr buffer )
    : DataIStream()
    , _impl( new detail::Command( buffer ))
{
    if( _impl->_buffer )
        *this >> _impl->_type >> _impl->_cmd;
}

Command::Command( const Command& rhs )
    : DataIStream()
    , _impl( new detail::Command( *rhs._impl ))
{
    if( _impl->_buffer )
    {
        // skip already set type & cmd to forward read pos
        get< uint32_t >();
        get< uint32_t >();
    }
}

Command& Command::operator = ( const Command& rhs )
{
    if( this != &rhs )
        *_impl = *rhs._impl;
    return *this;
}

Command::~Command()
{
    delete _impl;
}

uint32_t Command::getType() const
{
    return _impl->_type;
}

uint32_t Command::getCommand() const
{
    return _impl->_cmd;
}

void Command::setType( const CommandType type )
{
    _impl->_type = type;
}

void Command::setCommand( const uint32_t cmd )
{
    _impl->_cmd = cmd;
}

void Command::setDispatchFunction( const Dispatcher::Func& func )
{
    LBASSERT( !_impl->_func.isValid( ));
    _impl->_func = func;
}

uint64_t Command::getSize() const
{
    LBASSERT( isValid( ));
    return _impl->_buffer->getSize();
}

size_t Command::nRemainingBuffers() const
{
    return _impl->_buffer ? 1 : 0;
}

uint128_t Command::getVersion() const
{
    return uint128_t( 0, 0 );
}

NodePtr Command::getMaster()
{
    return getNode();
}

bool Command::getNextBuffer( uint32_t* compressor, uint32_t* nChunks,
                             const void** chunkData, uint64_t* size )
{
    if( !_impl->_buffer )
        return false;

    *chunkData = _impl->_buffer->getData();
    *size = _impl->_buffer->getSize();
    *compressor = EQ_COMPRESSOR_NONE;
    *nChunks = 1;

    return true;
}

NodePtr Command::getNode() const
{
    return _impl->_buffer->getNode();
}

LocalNodePtr Command::getLocalNode() const
{
    return _impl->_buffer->getLocalNode();
}

bool Command::isValid() const
{
    return _impl->_buffer && _impl->_buffer->isValid() &&
           _impl->_type != COMMANDTYPE_INVALID && _impl->_cmd != CMD_INVALID;
}

bool Command::operator()()
{
    LBASSERT( _impl->_func.isValid( ));
    Dispatcher::Func func = _impl->_func;
    _impl->_func.clear();
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

    if( command._impl->_func.isValid( ))
        os << ' ' << command._impl->_func << std::endl;
    return os;
}
}
