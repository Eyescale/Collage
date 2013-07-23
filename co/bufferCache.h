
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *
 * This file is part of Collage <https://github.com/Eyescale/Collage>
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

#ifndef CO_BUFFERCACHE_H
#define CO_BUFFERCACHE_H

#include <co/api.h>
#include <co/types.h>

#include <lunchbox/thread.h> // thread-safety checks

namespace co
{
namespace detail { class BufferCache; }

    /**
     * The buffer cache handles the reuse of allocated buffers for a node.
     *
     * Buffers are retained and released whenever they are not directly
     * processed, e.g., when pushed to another thread using a CommandQueue.
     */
    class BufferCache
    {
    public:
        CO_API BufferCache( const ssize_t minFree );
        CO_API ~BufferCache();

        /** @return a new buffer. */
        CO_API BufferPtr alloc( const uint64_t reserve );

        /** Compact buffer if too many commands are free. */
        void compact();

        /** Flush all allocated buffers. */
        void flush();

    private:
        detail::BufferCache* const _impl;
        friend std::ostream& operator << ( std::ostream&, const BufferCache& );
        LB_TS_VAR( _thread );
    };

    std::ostream& operator << ( std::ostream&, const BufferCache& );
}

#endif //CO_BUFFERCACHE_H
