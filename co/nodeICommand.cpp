
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

#include "nodeICommand.h"

#include "command.h"
#include "plugins/compressorTypes.h"


namespace co
{

namespace detail
{

class NodeICommand
{
public:
    CommandPtr command;
};

}

NodeICommand::NodeICommand( CommandPtr command )
    : DataIStream()
    , _impl( new detail::NodeICommand )
{
    _impl->command = command;

    PacketType type;
    uint32_t cmd;
    *this >> type >> cmd;
    (*command)->type = type;
    (*command)->command = cmd;
}

NodeICommand::~NodeICommand()
{
    delete _impl;
}

size_t NodeICommand::nRemainingBuffers() const
{
    return _impl->command ? 1 : 0;
}

uint128_t NodeICommand::getVersion() const
{
    return uint128_t( 0, 0 );
}

NodePtr NodeICommand::getMaster()
{
    return _impl->command ? _impl->command->getNode() : 0;
}

bool NodeICommand::getNextBuffer( uint32_t* compressor, uint32_t* nChunks,
                                  const void** chunkData, uint64_t* size )
{
    if( !_impl->command )
        return false;

    const CommandPtr command = _impl->command;
    _impl->command = 0;

    const uint8_t* ptr = reinterpret_cast< const uint8_t* >(
        command->get< Packet >()) + sizeof( uint64_t );
    *chunkData = ptr;

    *size = (*command)->size;
    *compressor = EQ_COMPRESSOR_NONE;
    *nChunks = 1;

    return true;
}

}
