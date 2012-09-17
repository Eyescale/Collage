
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
        , remote()
        , local()
    {}

    lunchbox::a_int32_t& freeCount;
    NodePtr remote; //!< The node sending the command
    LocalNodePtr local; //!< The node receiving the command
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
    return _impl->remote;
}

LocalNodePtr Buffer::getLocalNode() const
{
    return _impl->local;
}

bool Buffer::isValid() const
{
    return getSize() > 0;
}

void Buffer::setNodes( LocalNodePtr local, NodePtr remote )
{
    _impl->local = local;
    _impl->remote = remote;
}

void Buffer::alloc( const uint64_t size )
{
    LB_TS_THREAD( _writeThread );
    LBASSERT( getRefCount() == 1 ); // caller BufferCache
    LBASSERT( _impl->freeCount > 0 );

    --_impl->freeCount;
    _impl->local = 0;
    _impl->remote = 0;

    reserve( LB_MAX( getCacheSize(), size ));
    resize( size );
}

void Buffer::free()
{
    LB_TS_THREAD( _writeThread );

    clear();

    _impl->local = 0;
    _impl->remote = 0;
}

void Buffer::deleteReferenced( const Referenced* object ) const
{
    Buffer* buffer = const_cast< Buffer* >( this );
    ++buffer->_impl->freeCount;
}

size_t Buffer::getMinSize()
{
    return 256;
}

size_t Buffer::getCacheSize()
{
    return 4096; // Bigger than minSize!
}

}
