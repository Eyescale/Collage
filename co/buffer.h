
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

/**  */
class Buffer : public lunchbox::Bufferb, public lunchbox::Referenced
{
public:
    Buffer( lunchbox::a_int32_t& freeCounter );

    virtual ~Buffer();

    NodePtr getNode() const;

    /** @internal @return true if the packet is no longer in use. */
    bool isFree() const { return getRefCount() == 0; }

    /** @internal @return the number of newly allocated bytes. */
    size_t alloc_( NodePtr node, const uint64_t size );

    uint64_t getDataSize() const;

    /** @internal @return true if the command has a valid packet. */
    bool isValid() const;

    /**
     * @internal Clone the from command into this command.
     *
     * The command will share all data but the dispatch function. The
     * command's allocation size will be 0 and it will never delete the
     * shared data. The command will release its reference to the from
     * command when it is released.
     */
    void clone_( BufferPtr from );

    void free(); //!< @internal

    /** @internal Set the function to which the packet is dispatched. */
    void setDispatchFunction( const Dispatcher::Func& func );

    Dispatcher::Func getDispatchFunction() const;

    static size_t getMinSize(); //! @internal

private:
    detail::Buffer* const _impl;
    LB_TS_VAR( _writeThread );

    virtual void deleteReferenced( const Referenced* object ) const;
};
}

#endif //CO_BUFFER_H
