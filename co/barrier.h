
/* Copyright (c) 2006-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2011, Cedric Stalder <cedric.stalder@gmail.com>
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

#ifndef CO_BARRIER_H
#define CO_BARRIER_H

#include <co/object.h>   // base class
#include <co/types.h>

namespace co
{
namespace detail { class Barrier; }

/**
 * A networked, versioned barrier.
 *
 * On a given LocalNode only one instance of a given barrier can be mapped,
 * i.e., multiple instances of the same barrier are currently not supported by
 * the implementation. Not intended to be subclassed.
 */
class Barrier : public Object
{
public:
#ifdef COLLAGE_V1_API
    /** @deprecated Does not register or map barrier. */
    CO_API Barrier( NodePtr master = 0, const uint32_t height = 0 );
#endif

    /**
     * Construct and register a new distributed barrier.
     *
     * Barriers are versioned, distributed objects. This constructor creates and
     * registers the barrier with its LocalNode. Other processes contributing to
     * the barrier use the other constructor to create and map an instance to
     * this master version.
     *
     * The master node will maintain the barrier state. It has to be reachable
     * from all other nodes participating in the barrier. If the master node
     * identifier is not an UUID, the local node is used as the master.
     *
     * Note that the node of the object master, i.e., the instance which is
     * registered through this constructor, and the barrier's master node may be
     * different. The barriers master node maintains the barrier state. The user
     * of the barrier has to ensure that the given master node has at least one
     * instance of the barrier.
     *
     * @param localNode the local node to register the barrier with.
     * @param masterNodeID the master node identifier.
     * @param height the initial group size for the barrier.
     * @sa isGood()
     * @version 1.1.1
     */
    CO_API Barrier( LocalNodePtr localNode,
                    const uint128_t& masterNodeID,
                    const uint32_t height = 0 );

    /**
     * Construct and join a distributed barrier.
     *
     * @param localNode the local node to map the barrier to
     * @param barrier the identifier and version of the barrier
     * @sa isGood()
     * @version 1.1.1
     */
    CO_API Barrier( LocalNodePtr localNode, const ObjectVersion& barrier );

    /** Destruct the barrier. @version 1.0 */
    CO_API virtual ~Barrier();

    /**
     * @name Data Access
     *
     * After a change, the barrier has to be committed and synced to the same
     * version on all nodes entering the barrier.
     */
    //@{
    /** @return true if the barrier was created successfully. @version 1.1.1 */
    bool isGood() const { return isAttached(); }

    /** Set the number of participants in the barrier. @version 1.0 */
    CO_API void setHeight( const uint32_t height );

    /** Add one participant to the barrier. @version 1.0 */
    CO_API void increase();

    /** @return the number of participants. @version 1.0 */
    CO_API uint32_t getHeight() const;
    //@}

    /** @name Operations */
    //@{
    /**
     * Enter the barrier, blocks until the barrier has been reached.
     *
     * The implementation currently assumes that the master node instance
     * also enters the barrier. If a timeout happens a timeout exception is
     * thrown.
     * @version 1.0
     */
    CO_API void enter( const uint32_t timeout = LB_TIMEOUT_INDEFINITE );
    //@}

protected:
    /** @internal */
    //@{
    void attach( const uint128_t& id, const uint32_t instanceID ) override;
    ChangeType getChangeType() const override { return DELTA; }

    void getInstanceData( DataOStream& os ) override;
    void applyInstanceData( DataIStream& is ) override;
    void pack( DataOStream& os ) override;
    void unpack( DataIStream& is ) override;
    //@}

private:
    detail::Barrier* const _impl;

    void _cleanup( const uint64_t time );
    void _sendNotify( const uint128_t& version, NodePtr node );

    /* The command handlers. */
    bool _cmdEnter( ICommand& command );
    bool _cmdEnterReply( ICommand& command );

    LB_TS_VAR( _thread );
};
}

#endif // CO_BARRIER_H
