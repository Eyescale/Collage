
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

#ifndef CO_BUFFER_H
#define CO_BUFFER_H

#include <lunchbox/buffer.h>        // base class
#include <lunchbox/referenced.h>    // base class

#include <co/types.h>
#include <co/dispatcher.h>          // for Dispatcher::Func


namespace co
{
namespace detail { class Buffer; }

/**
 * The buffer containing the data of a co::Command.
 *
 * The command is required to extract the data from the buffer. The dispatching
 * methods are using the buffer for data forwarding because of the unknown
 * command type. The concrete command can be then instaniated with this buffer.
 *
 * The allocation of the buffer is always performed by the co::BufferCache. The
 * buffer API is supposed for internal use only.
*/
class Buffer : public lunchbox::Bufferb, public lunchbox::Referenced
{
public:
    Buffer( lunchbox::a_int32_t& freeCounter ); //!< @internal
    virtual ~Buffer(); //!< @internal

    /** @internal @return the sending node proxy instance. */
    NodePtr getNode() const;

    /** @internal @return the receiving node. */
    LocalNodePtr getLocalNode() const;

    /** @internal @return true if the buffer has valid data. */
    CO_API bool isValid() const;

    /** @internal @return true if the buffer is no longer in use. */
    bool isFree() const { return getRefCount() == 0; }

    /** @internal @return the number of newly allocated bytes. */
    size_t alloc( NodePtr node, LocalNodePtr localNode, const uint64_t size );

    void free(); //!< @internal

    static size_t getMinSize(); //! @internal

private:
    detail::Buffer* const _impl;
    LB_TS_VAR( _writeThread );

    virtual void deleteReferenced( const Referenced* object ) const;
};
}

#endif //CO_BUFFER_H
