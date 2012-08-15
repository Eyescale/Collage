
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com> 
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

#include "command.h"
#include "commands.h"
#include "objectDataICommand.h"

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
    _usedCommand = 0;
    _commands.clear();
    _version = VERSION_INVALID;
}

void ObjectDataIStream::addDataPacket( CommandPtr command )
{
    LB_TS_THREAD( _thread );
    LBASSERT( !isReady( ));

    ObjectDataICommand stream( command );
#ifndef NDEBUG    
    const uint128_t& version = stream.getVersion();
    const uint32_t sequence = stream.getSequence();

    if( _commands.empty( ))
    {
        LBASSERT( sequence == 0 );
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
    if( stream.isLast( ))
        _setReady();
}

bool ObjectDataIStream::hasInstanceData() const
{
    if( !_usedCommand && _commands.empty( ))
    {
        LBUNREACHABLE;
        return false;
    }

    const CommandPtr command = _usedCommand ? _usedCommand : _commands.front();
    return( (*command)->command == CMD_OBJECT_INSTANCE );
}

NodePtr ObjectDataIStream::getMaster()
{
    if( !_usedCommand && _commands.empty( ))
        return 0;

    const CommandPtr command = _usedCommand ? _usedCommand : _commands.front();
    return command->getNode();
}

size_t ObjectDataIStream::getDataSize() const
{
    size_t size = 0;
    for( CommandDequeCIter i = _commands.begin(); i != _commands.end(); ++i )
    {
        const CommandPtr command = *i;
        ObjectDataICommand stream( command );
        size += stream.getDataSize();
    }
    return size;
}

uint128_t ObjectDataIStream::getPendingVersion() const
{
    if( _commands.empty( ))
        return VERSION_INVALID;

    const CommandPtr command = _commands.back();
    ObjectDataICommand stream( command );
    return stream.getVersion();
}

const CommandPtr ObjectDataIStream::getNextCommand()
{
    if( _commands.empty( ))
        _usedCommand = 0;
    else
    {
        _usedCommand = _commands.front();
        _commands.pop_front();
    }
    return _usedCommand;
}

bool ObjectDataIStream::getNextBuffer( uint32_t* compressor, uint32_t* nChunks,
                                       const void** chunkData, uint64_t* size )
{
    const CommandPtr command = getNextCommand();
    if( !command )
        return false;

    ObjectDataICommand stream( command );

    LBASSERT( stream.getCommand() == CMD_OBJECT_INSTANCE ||
              stream.getCommand() == CMD_OBJECT_DELTA ||
              stream.getCommand() == CMD_OBJECT_SLAVE_DELTA );

    const uint64_t dataSize = stream.getDataSize();

    if( dataSize == 0 ) // empty packet
        return getNextBuffer( compressor, nChunks, chunkData, size );

    *size = dataSize;
    *compressor = stream.getCompressor();
    *nChunks = stream.getChunks();
    switch( stream.getCommand( ))
    {
    case CMD_OBJECT_INSTANCE:
        stream.get< NodeID >();
        stream.get< uint32_t >();
        break;
    case CMD_OBJECT_SLAVE_DELTA:
        stream.get< UUID >();
        break;
    }
    *chunkData = stream.getRemainingBuffer( dataSize );

    return true;
}

}
