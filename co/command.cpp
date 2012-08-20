
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
    Command( BufferPtr buffer )
        : _func( 0, 0 )
        , _buffer( buffer )
        , _type()
        , _cmd()
    {}

    Command( const Command& rhs )
        : _func( rhs._func )
        , _buffer( rhs._buffer )
        , _type()
        , _cmd()
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
    CommandType _type;
    uint32_t _cmd;
};
}

Command::Command( BufferPtr buffer )
    : DataIStream( )
    , _impl( new detail::Command( buffer ))
{
    if( _impl->_buffer )
        *this >> _impl->_type >> _impl->_cmd;
}

Command::Command( const Command& rhs )
    : DataIStream( )
    , _impl( new detail::Command( *rhs._impl ))
{
    if( _impl->_buffer )
        *this >> _impl->_type >> _impl->_cmd;
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

CommandType Command::getType() const
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
    memcpy( _impl->_buffer->getData(), &type, sizeof( type ));
}

void Command::setCommand( const uint32_t cmd )
{
    _impl->_cmd = cmd;
    memcpy( _impl->_buffer->getData() + sizeof( CommandType ),
            &cmd, sizeof( cmd ));
}

BufferPtr Command::getBuffer()
{
    return _impl->_buffer;
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
    *size = _impl->_buffer->getDataSize();
    *compressor = EQ_COMPRESSOR_NONE;
    *nChunks = 1;

    return true;
}

NodePtr Command::getNode() const
{
    return _impl->_buffer->getNode();
}

bool Command::isValid() const
{
    return _impl->_buffer;
}

bool Command::operator()()
{
    Dispatcher::Func func = _impl->_buffer->getDispatchFunction();
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
