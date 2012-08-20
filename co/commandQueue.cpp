
/* Copyright (c) 2005-2012, Stefan Eilemann <eile@equalizergraphics.com> 
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

#include "commandQueue.h"

#include "buffer.h"
#include "exception.h"
#include "node.h"

#include <lunchbox/mtQueue.h>

namespace co
{
namespace detail
{
class CommandQueue
{
public:
    /** Thread-safe buffer queue. */
    lunchbox::MTQueue< BufferPtr > buffers;
};
}

CommandQueue::CommandQueue()
        : _impl( new detail::CommandQueue )
{
}

CommandQueue::~CommandQueue()
{
    flush();
    delete _impl;
}

void CommandQueue::flush()
{
    if( !isEmpty( ))
        LBWARN << "Flushing non-empty command queue" << std::endl;

    _impl->buffers.clear();
}

bool CommandQueue::isEmpty() const
{
    return _impl->buffers.isEmpty();
}

size_t CommandQueue::getSize() const
{
    return _impl->buffers.getSize();
}

void CommandQueue::push( BufferPtr buffer )
{
    _impl->buffers.push( buffer );
}

void CommandQueue::pushFront( BufferPtr buffer )
{
    LBASSERT( buffer->isValid( ));
    _impl->buffers.pushFront( buffer );
}

BufferPtr CommandQueue::pop( const uint32_t timeout )
{
    LB_TS_THREAD( _thread );

    BufferPtr buffer;
    if( !_impl->buffers.timedPop( timeout, buffer ))
        throw Exception( Exception::TIMEOUT_COMMANDQUEUE );

    return buffer;
}

BufferPtr CommandQueue::tryPop()
{
    LB_TS_THREAD( _thread );
    BufferPtr buffer;
    _impl->buffers.tryPop( buffer );
    return buffer;
}

}

