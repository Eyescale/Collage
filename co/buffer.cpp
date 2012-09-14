
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2012, Stefan.Eilemann@epfl.ch
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

#include "buffer.h"

#include "localNode.h"
#include "node.h"


namespace co
{
namespace detail
{
class Buffer
{
public:
    Buffer( lunchbox::a_int32_t& freeCounter )
        : freeCount( freeCounter )
        , node()
        , localNode()
    {}

    lunchbox::a_int32_t& freeCount;
    NodePtr node; //!< The node sending the command
    LocalNodePtr  localNode; //!< The node receiving the command
};
}

Buffer::Buffer( lunchbox::a_int32_t& freeCounter )
    : lunchbox::Bufferb()
    , lunchbox::Referenced()
    , _impl( new detail::Buffer( freeCounter ))
{
}

Buffer::~Buffer()
{
    free();
    delete _impl;
}

NodePtr Buffer::getNode() const
{
    return _impl->node;
}

LocalNodePtr Buffer::getLocalNode() const
{
    return _impl->localNode;
}

bool Buffer::needsSwapping() const
{
    return _impl->node->isBigEndian() != _impl->localNode->isBigEndian();
}

bool Buffer::isValid() const
{
    return getSize() > 0;
}

void Buffer::alloc( NodePtr node, LocalNodePtr localNode, const uint64_t size)
{
    LB_TS_THREAD( _writeThread );
    LBASSERT( getRefCount() == 1 ); // caller BufferCache
    LBASSERT( _impl->freeCount > 0 );

    --_impl->freeCount;
    _impl->node = node;
    _impl->localNode = localNode;

    reserve( LB_MAX( getMinSize(), size ));
    resize( size );
}

void Buffer::free()
{
    LB_TS_THREAD( _writeThread );

    clear();

    _impl->node = 0;
    _impl->localNode = 0;
}

void Buffer::deleteReferenced( const Referenced* object ) const
{
    Buffer* buffer = const_cast< Buffer* >( this );
    ++buffer->_impl->freeCount;
}

size_t Buffer::getMinSize()
{
    return 4096;
}

}
