
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

#ifndef CO_BUFFER_H
#define CO_BUFFER_H

#include <lunchbox/buffer.h>        // base class
#include <lunchbox/referenced.h>    // base class

#include <co/api.h>
#include <co/types.h>
#include <co/dispatcher.h>          // for Dispatcher::Func


namespace co
{
namespace detail { class Buffer; }

/**
 * A receive buffer, containing the data for a co::ICommand.
 *
 * The buffer does not auto-delete, that is, a BufferPtr is not a smart
 * pointer. The BufferCache uses the BufferListener interface to reuse buffers
 * which are unreferenced, i.e., unused by any ICommand.
 */
class Buffer : public lunchbox::Bufferb, public lunchbox::Referenced
{
public:
    CO_API Buffer( BufferListener* listener = 0 );
    CO_API virtual ~Buffer();

    /** @return true if the buffer is no longer in use. */
    bool isFree() const { return getRefCount() == 0; }

    CO_API static size_t getMinSize(); //!< Size of first read on receiver
    CO_API static size_t getCacheSize(); //!< 'small' CommandCache allocation size

private:
    detail::Buffer* const _impl;
    LB_TS_VAR( _writeThread );

    virtual void deleteReferenced( const Referenced* object ) const;
};

std::ostream& operator << ( std::ostream&, const Buffer& );
}

#endif //CO_BUFFER_H
