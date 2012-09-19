
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "objectDataIStream.h"

#include "commands.h"
#include "objectDataCommand.h"

#include <co/plugins/compressor.h>

namespace co
{
ObjectDataIStream::ObjectDataIStream()
{
    _reset();
}

ObjectDataIStream::~ObjectDataIStream()
{
    _reset();
}

ObjectDataIStream::ObjectDataIStream( const ObjectDataIStream& from )
        : DataIStream( from )
        , _commands( from._commands )
        , _version( from._version )
{
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

void ObjectDataIStream::addDataCommand( ObjectDataCommand command )
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
        ObjectDataCommand previous( _commands.back() );
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

    const Command& command = _usedCommand.isValid() ? _usedCommand :
                                                      _commands.front();
    return( command.getCommand() == CMD_OBJECT_INSTANCE );
}

NodePtr ObjectDataIStream::getMaster()
{
    if( !_usedCommand.isValid() && _commands.empty( ))
        return 0;

    const Command& command = _usedCommand.isValid() ? _usedCommand :
                                                      _commands.front();
    return command.getNode();
}

size_t ObjectDataIStream::getDataSize() const
{
    size_t size = 0;
    for( CommandDeque::const_iterator i = _commands.begin();
         i != _commands.end(); ++i )
    {
        const Command& command = *i;
        size += command.getSize();
    }
    return size;
}

uint128_t ObjectDataIStream::getPendingVersion() const
{
    if( _commands.empty( ))
        return VERSION_INVALID;

    const ObjectDataCommand& cmd( _commands.back( ));
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

    ObjectDataCommand command( _usedCommand );
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
        command.get< UUID >();      // commit UUID
        break;
    }
    *chunkData = command.getRemainingBuffer( command.getRemainingBufferSize( ));

    setSwapping( command.isSwapping( ));
    return true;
}

}
