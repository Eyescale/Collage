
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
        : _freeCount( freeCounter )
        , _node()
        , _localNode()
    {}

    lunchbox::a_int32_t& _freeCount;
    NodePtr _node; //!< The node sending the command
    LocalNodePtr  _localNode; //!< The node receiving the command
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
    return _impl->_node;
}

LocalNodePtr Buffer::getLocalNode() const
{
    return _impl->_localNode;
}

bool Buffer::isValid() const
{
    return getSize() > 0;
}

size_t Buffer::alloc( NodePtr node, LocalNodePtr localNode, const uint64_t size)
{
    LB_TS_THREAD( _writeThread );
    LBASSERT( getRefCount() == 1 ); // caller BufferCache
    LBASSERT( _impl->_freeCount > 0 );

    --_impl->_freeCount;
    _impl->_node = node;
    _impl->_localNode = localNode;

    reserve( LB_MAX( getMinSize(), size ));
    resize( size );

    return getSize();
}

void Buffer::free()
{
    LB_TS_THREAD( _writeThread );

    clear();

    _impl->_node = 0;
    _impl->_localNode = 0;
}

void Buffer::deleteReferenced( const Referenced* object ) const
{
    Buffer* buffer = const_cast< Buffer* >( this );
    ++buffer->_impl->_freeCount;
}

size_t Buffer::getMinSize()
{
    return 4096;
}

}
