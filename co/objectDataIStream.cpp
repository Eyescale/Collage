
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

#include "buffer.h"
#include "command.h"
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
        , _buffers( from._buffers )
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
    _usedBuffer = 0;
    _buffers.clear();
    _version = VERSION_INVALID;
}

void ObjectDataIStream::addDataPacket( BufferPtr buffer )
{
    LB_TS_THREAD( _thread );
    LBASSERT( !isReady( ));

    ObjectDataCommand command( buffer );
#ifndef NDEBUG
    const uint128_t& version = command.getVersion();
    const uint32_t sequence = command.getSequence();

    if( _buffers.empty( ))
    {
        LBASSERT( sequence == 0 );
    }
    else
    {
        ObjectDataCommand previous( _buffers.back() );
        const uint128_t& previousVersion = previous.getVersion();
        const uint32_t previousSequence = previous.getSequence();
        LBASSERTINFO( sequence == previousSequence+1,
                      sequence << ", " << previousSequence );
        LBASSERT( version == previousVersion );
    }
#endif

    _buffers.push_back( buffer );
    if( command.isLast( ))
        _setReady();
}

bool ObjectDataIStream::hasInstanceData() const
{
    if( !_usedBuffer && _buffers.empty( ))
    {
        LBUNREACHABLE;
        return false;
    }

    const BufferPtr buffer = _usedBuffer ? _usedBuffer : _buffers.front();
    ObjectDataCommand cmd( buffer );
    return( cmd.getCommand() == CMD_OBJECT_INSTANCE );
}

NodePtr ObjectDataIStream::getMaster()
{
    if( !_usedBuffer && _buffers.empty( ))
        return 0;

    const BufferPtr buffer = _usedBuffer ? _usedBuffer : _buffers.front();
    return buffer->getNode();
}

size_t ObjectDataIStream::getDataSize() const
{
    size_t size = 0;
    for( BufferDequeCIter i = _buffers.begin(); i != _buffers.end(); ++i )
    {
        const BufferPtr buffer = *i;
        size += buffer->getDataSize();
    }
    return size;
}

uint128_t ObjectDataIStream::getPendingVersion() const
{
    if( _buffers.empty( ))
        return VERSION_INVALID;

    const BufferPtr buffer = _buffers.back();
    ObjectDataCommand cmd( buffer );
    return cmd.getVersion();
}

const BufferPtr ObjectDataIStream::_getNextBuffer()
{
    if( _buffers.empty( ))
        _usedBuffer = 0;
    else
    {
        _usedBuffer = _buffers.front();
        _buffers.pop_front();
    }
    return _usedBuffer;
}

bool ObjectDataIStream::getNextBuffer( uint32_t* compressor, uint32_t* nChunks,
                                       const void** chunkData, uint64_t* size )
{
    BufferPtr buffer = _getNextBuffer();
    if( !buffer )
        return false;

    ObjectDataCommand command( buffer );

    LBASSERT( command.getCommand() == CMD_OBJECT_INSTANCE ||
              command.getCommand() == CMD_OBJECT_DELTA ||
              command.getCommand() == CMD_OBJECT_SLAVE_DELTA );

    const uint64_t dataSize = command.getDataSize();

    if( dataSize == 0 ) // empty packet
        return getNextBuffer( compressor, nChunks, chunkData, size );

    *size = dataSize;
    *compressor = command.getCompressor();
    *nChunks = command.getChunks();
    switch( command.getCommand( ))
    {
    case CMD_OBJECT_INSTANCE:
        command.get< NodeID >();
        command.get< uint32_t >();
        break;
    case CMD_OBJECT_SLAVE_DELTA:
        command.get< UUID >();
        break;
    }
    *chunkData = command.getRemainingBuffer( command.getRemainingBufferSize( ));

    return true;
}

}
