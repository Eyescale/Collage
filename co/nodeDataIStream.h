
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

#ifndef CO_NODEDATAISTREAM_H
#define CO_NODEDATAISTREAM_H

#include <co/dataIStream.h>   // base class


namespace co
{

namespace detail { class NodeDataIStream; }

/** The DataIStream for node data & commands. */
class NodeDataIStream : public DataIStream
{
public:
    NodeDataIStream( CommandPtr command );
    virtual ~NodeDataIStream();

protected:
    virtual size_t nRemainingBuffers() const;

    virtual uint128_t getVersion() const;

    virtual NodePtr getMaster();

    virtual bool getNextBuffer( uint32_t* compressor, uint32_t* nChunks,
                                const void** chunkData, uint64_t* size );

private:
    detail::NodeDataIStream* const _impl;
};
}

#endif //CO_NODEDATAISTREAM_H
