
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
#include <co/dispatcher.h>


namespace co
{
namespace detail { class Buffer; }

// #145 Documentation & API
/**  */
class Buffer : public lunchbox::Bufferb, public lunchbox::Referenced
{
public:
    Buffer( lunchbox::a_int32_t& freeCounter ); //!< @internal
    virtual ~Buffer(); //!< @internal

    /** @internal */
    NodePtr getNode() const;

    /** @internal */
    LocalNodePtr getLocalNode() const;

    /** @internal @return the actual size of the buffers content. */
    uint64_t getDataSize() const;

    /** @internal @return true if the buffer has a valid data. */
    CO_API bool isValid() const;

    /** @internal @return true if the buffer is no longer in use. */
    bool isFree() const { return getRefCount() == 0; }

    /** @internal @return the number of newly allocated bytes. */
    size_t alloc( NodePtr node, LocalNodePtr localNode, const uint64_t size );

    /**
     * @internal Clone the from buffer into this buffer.
     *
     * The command will share all data but the dispatch function. The
     * command's allocation size will be 0 and it will never delete the
     * shared data. The command will release its reference to the from
     * command when it is released.
     */
    void clone( BufferPtr from );

    void free(); //!< @internal

    /** @internal Set the function to which the buffer is dispatched. */
    void setDispatchFunction( const Dispatcher::Func& func );

    /** @internal @return the function to which the buffer is dispatched. */
    Dispatcher::Func getDispatchFunction() const;

    static size_t getMinSize(); //! @internal

private:
    detail::Buffer* const _impl;
    LB_TS_VAR( _writeThread );

    virtual void deleteReferenced( const Referenced* object ) const;
};
}

#endif //CO_BUFFER_H
