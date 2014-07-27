
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "objectDataIStream.h"

#include "commands.h"
#include "objectCommand.h"
#include "objectDataICommand.h"

namespace co
{
ObjectDataIStream::ObjectDataIStream()
    : DataIStream( false )
{
    _reset();
}

ObjectDataIStream::ObjectDataIStream( const ObjectDataIStream& rhs )
    : DataIStream( rhs )
{
    *this = rhs;
}

ObjectDataIStream& ObjectDataIStream::operator = ( const ObjectDataIStream& rhs)
{
    if( this != &rhs )
    {
        DataIStream::operator = ( rhs );
        _commands = rhs._commands;
        _version = rhs._version;
    }
    return *this;
}

ObjectDataIStream::~ObjectDataIStream()
{
    _reset();
}

void ObjectDataIStream::reset()
{
    DataIStream::reset();
    _reset();
}

void ObjectDataIStream::_reset()
{
    _usedCommand.clear();
    _commands.clear();
    _version = VERSION_INVALID;
}

void ObjectDataIStream::addDataCommand( ObjectDataICommand command )
{
    LB_TS_THREAD( _thread );
    LBASSERT( !isReady( ));

#ifndef NDEBUG
    const uint128_t& version = command.getVersion();
    const uint32_t sequence = command.getSequence();

    if( _commands.empty( ))
    {
        LBASSERTINFO( sequence == 0, sequence << " in " << command );
    }
    else
    {
        ObjectDataICommand previous( _commands.back() );
        const uint128_t& previousVersion = previous.getVersion();
        const uint32_t previousSequence = previous.getSequence();
        LBASSERTINFO( sequence == previousSequence+1,
                      sequence << ", " << previousSequence );
        LBASSERT( version == previousVersion );
    }
#endif

    _commands.push_back( command );
    if( command.isLast( ))
        _setReady();
}

bool ObjectDataIStream::hasInstanceData() const
{
    if( !_usedCommand.isValid() && _commands.empty( ))
    {
        LBUNREACHABLE;
        return false;
    }

    const ICommand& command = _usedCommand.isValid() ? _usedCommand :
                                                      _commands.front();
    return( command.getCommand() == CMD_OBJECT_INSTANCE );
}

NodePtr ObjectDataIStream::getRemoteNode() const
{
    if( !_usedCommand.isValid() && _commands.empty( ))
        return 0;

    const ICommand& command = _usedCommand.isValid() ? _usedCommand :
                                                      _commands.front();
    return command.getRemoteNode();
}

LocalNodePtr ObjectDataIStream::getLocalNode() const
{
    if( !_usedCommand.isValid() && _commands.empty( ))
        return 0;

    const ICommand& command = _usedCommand.isValid() ? _usedCommand :
                                                      _commands.front();
    return command.getLocalNode();
}

size_t ObjectDataIStream::getDataSize() const
{
    size_t size = 0;
    typedef CommandDeque::const_iterator CommandDequeCIter;
    for( CommandDequeCIter i = _commands.begin(); i != _commands.end(); ++i )
    {
        const ICommand& command = *i;
        size += command.getSize();
    }
    return size;
}

uint128_t ObjectDataIStream::getPendingVersion() const
{
    if( _commands.empty( ))
        return VERSION_INVALID;

    const ObjectDataICommand& cmd( _commands.back( ));
    return cmd.getVersion();
}

bool ObjectDataIStream::getNextBuffer( uint32_t& compressor, uint32_t& nChunks,
                                       const void** chunkData, uint64_t& size )
{
    if( _commands.empty( ))
    {
        _usedCommand.clear();
        return false;
    }

    _usedCommand = _commands.front();
    _commands.pop_front();
    if( !_usedCommand.isValid( ))
        return false;

    LBASSERT( _usedCommand.getCommand() == CMD_OBJECT_INSTANCE ||
              _usedCommand.getCommand() == CMD_OBJECT_DELTA ||
              _usedCommand.getCommand() == CMD_OBJECT_SLAVE_DELTA );

    ObjectDataICommand command( _usedCommand );
    const uint64_t dataSize = command.getDataSize();

    if( dataSize == 0 ) // empty command
        return getNextBuffer( compressor, nChunks, chunkData, size );

    size = dataSize;
    compressor = command.getCompressor();
    nChunks = command.getChunks();
    switch( command.getCommand( ))
    {
      case CMD_OBJECT_INSTANCE:
        command.get< NodeID >();    // nodeID
        command.get< uint32_t >();  // instanceID
        break;
      case CMD_OBJECT_SLAVE_DELTA:
        command.get< uint128_t >(); // commit UUID
        break;
    }
    *chunkData = command.getRemainingBuffer( command.getRemainingBufferSize( ));

    setSwapping( command.isSwapping( ));
    return true;
}

}
