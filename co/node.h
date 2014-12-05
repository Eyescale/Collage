
/* Copyright (c) 2005-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
 *               2012-2014, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_NODE_H
#define CO_NODE_H

#include <co/dispatcher.h>        // base class
#include <co/connection.h>        // used in inline template method
#include <co/nodeType.h>          // for NODETYPE_NODE enum
#include <co/types.h>

namespace co
{
namespace detail { class Node; }

/**
 * Proxy node representing a remote LocalNode.
 *
 * A node represents a separate entity in a peer-to-peer network, typically a
 * process on another machine. It should have at least one Connection through
 * which is reachable. A Node provides the basic communication facilities
 * through message passing.
 */
class Node : public Dispatcher, public lunchbox::Referenced
{
public:
    /**
     * Construct a new node proxy.
     *
     * @param type the type of the node, used during connect().
     * @version 1.0
     */
    CO_API explicit Node( const uint32_t type = co::NODETYPE_NODE );

    /** @name Data Access */
    //@{
    /**
     * Get the node's unique identifier.
     *
     * In rare cases (two nodes initiate a two-sided LocalNode::connect()) to
     * each other, two node instances with the same identifier might exist
     * temporarily during the connection handshake.
     *
     * @return the node's unique identifier.
     * @version 1.0
     */
    CO_API const NodeID& getNodeID() const;

    /** @return the type of the node. @version 1.0 */
    CO_API uint32_t getType() const;

    bool operator == ( const Node* n ) const; //!< @internal
    bool isBigEndian() const; //!< @internal

    /** @return true if the node can send/receive messages. @version 1.0 */
    CO_API bool isReachable() const;

    /** @return true if the remote node is reachable. @version 1.0 */
    CO_API bool isConnected() const;

    /** @return true if the local node is reachable. @version 1.0 */
    CO_API bool isListening() const;

    /** @return true if then node is not active. @version 1.0 */
    CO_API bool isClosed() const;

    /** @return true if the node is about to become inactive. @version 1.0*/
    CO_API bool isClosing() const;
    //@}

    /** @name Connectivity information */
    //@{
    /** @return true if the node is local (listening). @version 1.0 */
    bool isLocal() const { return isListening(); }

    /**
     * Add a new description how this node can be reached.
     *
     * The node has to be closed.
     *
     * @param cd the connection description.
     * @version 1.0
     */
    CO_API void addConnectionDescription( ConnectionDescriptionPtr cd );

    /**
     * Removes a connection description.
     *
     * The node has to be closed.
     *
     * @param cd the connection description.
     * @return true if the connection description was removed, false otherwise.
     * @version 1.0
     */
    CO_API bool removeConnectionDescription( ConnectionDescriptionPtr cd );

    /** @return the connection descriptions. @version 1.0 */
    CO_API ConnectionDescriptions getConnectionDescriptions() const;

    /**
     * Get an active connection to this node.
     *
     * @param multicast if true, prefer a multicast connection.
     * @return an active connection to this node.
     * @version 1.0
     */
    CO_API ConnectionPtr getConnection( const bool multicast = false );
    //@}

    /** @name Messaging API */
    //@{
    /**
     * Send a command with optional data to the node.
     *
     * The returned command can be used to pass additional data. The data will
     * be send after the command object is destroyed, aka when it is running out
     * of scope. Thread safe.
     *
     * @param cmd the node command to execute.
     * @param multicast prefer multicast connection for sending.
     * @return the command object to append additional data.
     * @version 1.0
     */
    CO_API OCommand send( const uint32_t cmd, const bool multicast = false);

    /**
     * Send a custom command with optional data to the node.
     *
     * The command handler for this command being send is registered with the
     * remote LocalNode::registerCommandHandler().
     *
     * The returned command can be used to pass additional data. The data will
     * be send after the command object is destroyed, aka when it is running out
     * of scope. Thread safe.
     *
     * @param commandID the ID of the registered custom command.
     * @param multicast prefer multicast connection for sending.
     * @return the command object to append additional data.
     * @version 1.0
     */
    CO_API CustomOCommand send( const uint128_t& commandID,
                                const bool multicast = false );
    //@}

    /** @internal @return last receive time. */
    CO_API int64_t getLastReceiveTime() const;

    /** @internal Serialize the node's information. */
    CO_API std::string serialize() const;
    /** @internal Deserialize the node information, consumes given data. */
    CO_API bool deserialize( std::string& data );

protected:
    /** Destruct this node. @version 1.0 */
    CO_API virtual ~Node();

    /** @internal */
    void _addConnectionDescription( ConnectionDescriptionPtr cd );
    /** @internal */
    bool _removeConnectionDescription( ConnectionDescriptionPtr cd );

    /** @internal @return the active multicast connection to this node. */
    ConnectionPtr _getMulticast() const;

    /**
     * Activate and return a multicast connection.
     *
     * Multicast connections are activated lazily on first use, since they
     * trigger the creation of the remote local node proxies on all members of
     * the multicast group.
     *
     * @return the first usable multicast connection to this node, or 0.
     * @version 1.0
     */
    ConnectionPtr getMulticast();

private:
    detail::Node* const _impl;
    CO_API friend std::ostream& operator << ( std::ostream&, const Node& );

    /** Ensures the connectivity of this node. */
    ConnectionPtr _getConnection( const bool preferMulticast );

    /** @internal @name Methods for LocalNode */
    //@{
    void _addMulticast( NodePtr node, ConnectionPtr connection );
    void _removeMulticast( ConnectionPtr connection );
    void _connectMulticast( NodePtr node );
    void _connectMulticast( NodePtr node, ConnectionPtr connection );
    void _setListening();
    void _setClosing();
    void _setClosed();
    void _connect( ConnectionPtr connection );
    void _disconnect();
    void _setLastReceive( const int64_t time );
    friend class LocalNode;
    //@}
};

CO_API std::ostream& operator << ( std::ostream& os, const Node& node );
}

namespace lunchbox
{
template<> inline void byteswap( co::Node*& ) { /*NOP*/ }
template<> inline void byteswap( co::NodePtr& ) { /*NOP*/ }
}

#endif // CO_NODE_H
