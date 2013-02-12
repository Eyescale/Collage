
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2012-2013, Stefan.Eilemann@epfl.ch
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

#ifndef CO_BUFFER_H
#define CO_BUFFER_H

#include <co/api.h>
#include <co/types.h>

#include <lunchbox/buffer.h>        // base class
#include <lunchbox/referenced.h>    // base class


namespace co
{
namespace detail { class Buffer; }

/**
 * A receive buffer for a Connection.
 *
 * The buffer does not auto-delete, that is, a BufferPtr is not a smart
 * pointer. Instead, the BufferListener interface notifies when a buffer is
 * reusable. The BufferCache uses this to recycle unreferenced buffers, i.e.,
 * unused by any ICommand.
 */
class Buffer : public lunchbox::Bufferb, public lunchbox::Referenced
{
public:
    /** Construct a new buffer. @version 1.0 */
    CO_API Buffer( BufferListener* listener = 0 );

    /** Destruct this buffer. @version 1.0 */
    CO_API virtual ~Buffer();

    /** @return true if the buffer is no longer in use. @version 1.0 */
    bool isFree() const { return getRefCount() == 0; }

private:
    detail::Buffer* const _impl;
    LB_TS_VAR( _writeThread );

    virtual void notifyFree();
};

std::ostream& operator << ( std::ostream&, const Buffer& );
}

#endif //CO_BUFFER_H
