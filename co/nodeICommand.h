
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

#ifndef CO_NODEICOMMAND_H
#define CO_NODEICOMMAND_H

#include <co/dataIStream.h>   // base class


namespace co
{

namespace detail { class NodeICommand; }

/** A DataIStream based command for co::Node. */
class NodeICommand : public DataIStream
{
public:
    NodeICommand( CommandPtr command );
    virtual ~NodeICommand();

protected:
    virtual size_t nRemainingBuffers() const;

    virtual uint128_t getVersion() const;

    virtual NodePtr getMaster();

    virtual bool getNextBuffer( uint32_t* compressor, uint32_t* nChunks,
                                const void** chunkData, uint64_t* size );

private:
    detail::NodeICommand* const _impl;
};
}

#endif //CO_NODEICOMMAND_H
