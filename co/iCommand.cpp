
/* Copyright (c) 2006-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *
 * This file is part of Collage <https://github.com/Eyescale/Collage>
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

#include "iCommand.h"

#include "buffer.h"
#include "localNode.h"
#include "node.h"
#include <lunchbox/plugins/compressorTypes.h>

namespace co
{
namespace detail
{
class ICommand
{
public:
    ICommand()
        : func( 0, 0 )
        , buffer( 0 )
        , size( 0 )
        , type( COMMANDTYPE_INVALID )
        , cmd( CMD_INVALID )
        , consumed( false )
    {}

    ICommand( LocalNodePtr local_, NodePtr remote_, ConstBufferPtr buffer_ )
        : local( local_ )
        , remote( remote_ )
        , func( 0, 0 )
        , buffer( buffer_ )
        , size( 0 )
        , type( COMMANDTYPE_INVALID )
        , cmd( CMD_INVALID )
        , consumed( false )
    {}

    void clear()
    {
        *this = ICommand();
    }

    LocalNodePtr local; //!< The node receiving the command
    NodePtr remote; //!< The node sending the command
    co::Dispatcher::Func func;
    ConstBufferPtr buffer;
    uint64_t size;
    uint32_t type;
    uint32_t cmd;
    bool consumed;
};
} // detail namespace

ICommand::ICommand()
    : DataIStream( false )
    , _impl( new detail::ICommand )
{
}

ICommand::ICommand( LocalNodePtr local, NodePtr remote, ConstBufferPtr buffer,
                  const bool swap_ )
    : DataIStream( swap_ )
    , _impl( new detail::ICommand( local, remote, buffer ))
{
    if( _impl->buffer )
        *this >> _impl->size >> _impl->type >> _impl->cmd;
}

ICommand::ICommand( const ICommand& rhs )
    : DataIStream( rhs )
    , _impl( new detail::ICommand( *rhs._impl ))
{
    _impl->consumed = false;
    _skipHeader();
}

ICommand& ICommand::operator = ( const ICommand& rhs )
{
    if( this != &rhs )
    {
        DataIStream::operator = ( rhs );
        *_impl = *rhs._impl;
        _impl->consumed = false;
        _skipHeader();
    }
    return *this;
}

ICommand::~ICommand()
{
    delete _impl;
}

void ICommand::clear()
{
    _impl->clear();
}

void ICommand::_skipHeader()
{
    const size_t headerSize = sizeof( _impl->size ) + sizeof( _impl->type ) +
                              sizeof( _impl->cmd );
    if( isValid() && getRemainingBufferSize() >= headerSize )
        getRemainingBuffer( headerSize );
}

uint32_t ICommand::getType() const
{
    return _impl->type;
}

uint32_t ICommand::getCommand() const
{
    return _impl->cmd;
}

uint64_t ICommand::getSize() const
{
    return _impl->size;
}

void ICommand::setType( const CommandType type )
{
    _impl->type = type;
}

void ICommand::setCommand( const uint32_t cmd )
{
    _impl->cmd = cmd;
}

void ICommand::setDispatchFunction( const Dispatcher::Func& func )
{
    _impl->func = func;
}

ConstBufferPtr ICommand::getBuffer() const
{
    LBASSERT( _impl->buffer );
    return _impl->buffer;
}

size_t ICommand::nRemainingBuffers() const
{
    return _impl->buffer ? 1 : 0;
}

uint128_t ICommand::getVersion() const
{
    return VERSION_NONE;
}

NodePtr ICommand::getMaster()
{
    return getNode();
}

bool ICommand::getNextBuffer( uint32_t& compressor, uint32_t& nChunks,
                             const void** chunkData, uint64_t& size )
{
    if( !_impl->buffer )
        return false;

#ifndef NDEBUG
    LBASSERT( !_impl->consumed );
    _impl->consumed = true;
#endif

    *chunkData = _impl->buffer->getData();
    size = _impl->buffer->getSize();
    compressor = EQ_COMPRESSOR_NONE;
    nChunks = 1;
    return true;
}

NodePtr ICommand::getNode() const
{
    return _impl->remote;
}

LocalNodePtr ICommand::getLocalNode() const
{
    return _impl->local;
}

bool ICommand::isValid() const
{
    return _impl->buffer && !_impl->buffer->isEmpty() &&
           _impl->type != COMMANDTYPE_INVALID && _impl->cmd != CMD_INVALID &&
           _impl->size > 0;
}

bool ICommand::operator()()
{
    LBASSERT( _impl->func.isValid( ));
    Dispatcher::Func func = _impl->func;
    _impl->func.clear();
    return func( *this );
}

std::ostream& operator << ( std::ostream& os, const ICommand& command )
{
    ConstBufferPtr buffer = command.getBuffer();
    if( buffer )
        os << lunchbox::disableFlush << "command< type "
           << uint32_t( command.getType( )) << " cmd " << command.getCommand()
           << " size " << command.getSize() << '/' << buffer->getSize() << '/'
           << buffer->getMaxSize() << " from " << command.getNode() << " to "
           << command.getLocalNode() << " >" << lunchbox::enableFlush;
    else
        os << "command< empty >";

    if( command._impl->func.isValid( ))
        os << ' ' << command._impl->func << std::endl;
    return os;
}
}
