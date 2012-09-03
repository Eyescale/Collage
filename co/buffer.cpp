
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
        , _master()
        , _func( 0, 0 )
    {}

    lunchbox::a_int32_t& _freeCount;
    NodePtr _node; //!< The node sending the packet
    LocalNodePtr  _localNode; //!< The node receiving the packet
    BufferPtr _master;
    co::Dispatcher::Func _func;
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
    LBASSERT( getRefCount() == 1 ); // caller CommandCache
    LBASSERT( _impl->_freeCount > 0 );

    // 'unclone' ourselves
    if( _impl->_master )
    {
        _data = 0;
        _size = 0;
        _maxSize = 0;
    }

    --_impl->_freeCount;
    _impl->_node = node;
    _impl->_localNode = localNode;
    _impl->_master = 0;

    reserve( LB_MAX( getMinSize(), size ));
    resize( size );

    return getSize();
}

void Buffer::clone( BufferPtr from )
{
    LB_TS_THREAD( _writeThread );
    LBASSERT( getRefCount() == 1 ); // caller CommandCache
    LBASSERT( from->getRefCount() > 1 ); // caller CommandCache, self
    LBASSERT( _impl->_freeCount > 0 );

    free();

    --_impl->_freeCount;

    _data = from->getData();
    _size = from->getSize();

    _impl->_node = from->_impl->_node;
    _impl->_localNode = from->_impl->_localNode;
    _impl->_master = from;
}

void Buffer::free()
{
    LB_TS_THREAD( _writeThread );

    // if this buffer is a clone, don't dare to free the owner's data
    if( _impl->_master )
    {
        _data = 0;
        _size = 0;
        _maxSize = 0;
    }
    else
        clear();

    _impl->_node = 0;
    _impl->_localNode = 0;
    _impl->_master = 0;
}

void Buffer::deleteReferenced( const Referenced* object ) const
{
    Buffer* buffer = const_cast< Buffer* >( this );
    // DON'T 'command->_master = 0;', command is already reusable and _master
    // may be set any time. alloc or clone_ will free old master.
    ++buffer->_impl->_freeCount;
}

void Buffer::setDispatchFunction( const Dispatcher::Func& func )
{
    _impl->_func = func;
}

Dispatcher::Func Buffer::getDispatchFunction() const
{
    LBASSERT( _impl->_func.isValid( ));
    return _impl->_func;
}

size_t Buffer::getMinSize()
{
    return 4096;
}

}
