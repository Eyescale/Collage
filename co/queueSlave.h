
/* Copyright (c) 2011-2014, Stefan Eilemann <eile@eyescale.ch>
 *                    2011, Carsten Rohn <carsten.rohn@rtt.ag>
 *               2011-2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_QUEUESLAVE_H
#define CO_QUEUESLAVE_H

#include <co/api.h>
#include <co/object.h> // base class
#include <co/types.h>

namespace co
{
namespace detail { class QueueSlave; }

/**
 * The consumer end of a distributed queue.
 *
 * One or more instances of this class are mapped to the identifier of the
 * QueueMaster registered on another node.
 */
class QueueSlave : public Object
{
public:
    /**
     * Construct a new queue consumer.
     *
     * The implementation will prefetch items from the queue master to cache
     * them locally. The prefetchMark determines when new items are requested,
     * and the prefetchAmount how many items are fetched. Prefetching items
     * hides the network latency by pipelining the network communication with
     * the processing, but introduces some imbalance between queue slaves.
     *
     * @param prefetchMark the low-water mark for prefetching, or
     *                     LB_UNDEFINED_UINT32 to use the Global default.
     * @param prefetchAmount the refill quantity when prefetching, or
     *                       LB_UNDEFINED_UINT32 to use the Global default.
     * @version 1.0
     */
    CO_API QueueSlave( const uint32_t prefetchMark = LB_UNDEFINED_UINT32,
                       const uint32_t prefetchAmount = LB_UNDEFINED_UINT32 );

    /** Destruct this queue consumer. @version 1.0 */
    virtual CO_API ~QueueSlave();

    /**
     * Dequeue an item.
     *
     * The returned item can deserialize additional data using the DataIStream
     * operators.
     *
     * @param timeout An optional timeout for the operation.
     * @return an item from the distributed queue, or an invalid item if the
     *         queue is empty or the operation timed out.
     * @version 1.0
     */
    CO_API ObjectICommand pop( const uint32_t timeout = LB_TIMEOUT_INDEFINITE );

protected:
    ChangeType getChangeType() const override { return STATIC; }
    void getInstanceData( co::DataOStream& ) override { LBDONTCALL }
    void applyInstanceData( co::DataIStream& is ) override;

private:
    detail::QueueSlave* const _impl;

    CO_API void attach( const uint128_t& id,
                        const uint32_t instanceID ) override;
};

} // co

#endif // CO_QUEUESLAVE_H
