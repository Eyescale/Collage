
/* Copyright (c) 2005-2014, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_LOCALNODE_H
#define CO_LOCALNODE_H

#include <co/node.h>            // base class
#include <co/objectHandler.h>   // base class
#include <co/objectVersion.h>   // VERSION_FOO used inline
#include <lunchbox/requestHandler.h> // base class

#include <boost/function/function1.hpp>
#include <boost/function/function4.hpp>

namespace co
{
namespace detail { class LocalNode; class ReceiverThread; class CommandThread; }

/**
 * Node specialization for a local node.
 *
 * Local nodes listen on network connections, manage connections to other nodes
 * and provide Object registration, mapping and command dispatch. Typically each
 * process uses one local node to communicate with other processes.
 */
class LocalNode : public lunchbox::RequestHandler, public Node,
                  public ObjectHandler
{
public:
    /**
     * Counters are monotonically increasing performance variables for
     * operations performed by a LocalNode instance.
     */
    enum Counter
    {
        COUNTER_MAP_OBJECT_REMOTE, //!< Num of mapObjects served for other nodes
        COUNTER_ALL // must be last
    };

    /** Construct a new local node of the given type. @version 1.0 */
    CO_API explicit LocalNode( const uint32_t type = co::NODETYPE_NODE );

    /**
     * @name State Changes
     *
     * The following methods affect the state of the node by changing its
     * connectivity to the network.
     */
    //@{
    /**
     * Initialize the node.
     *
     * Parses the following command line options and calls listen()
     * afterwards:
     *
     * The '--co-listen &lt;connection description&gt;' command line option is
     * parsed by this method to add listening connections to this node. This
     * parameter might be used multiple times.
     * ConnectionDescription::fromString() is used to parse the provided
     * description.
     *
     * The '--co-globals &lt;string&gt;' option is used to initialize the
     * Globals. The string is parsed used Globals::fromString().
     *
     * Please note that further command line parameters are recognized by
     * co::init().
     *
     * @param argc the command line argument count.
     * @param argv the command line argument values.
     * @return true if the client was successfully initialized, false otherwise.
     * @version 1.0
     */
    CO_API virtual bool initLocal( const int argc, char** argv );

    /**
     * Open all connections and put this node into the listening state.
     *
     * The node will spawn a receiver and command thread, and listen on all
     * connections for incoming commands. The node will be in the listening
     * state if the method completed successfully. A listening node can connect
     * other nodes.
     *
     * @return true if the node could be initialized, false otherwise.
     * @sa connect()
     * @version 1.0
     */
    CO_API virtual bool listen();

    /**
     * Close a listening node.
     *
     * Disconnects all connected node proxies, closes the listening connections
     * and terminates all threads created in listen().
     *
     * @return true if the node was stopped, false otherwise.
     * @version 1.0
     */
    CO_API virtual bool close();

    /** Close a listening node. @version 1.0 */
    virtual bool exitLocal() { return close(); }

    /**
     * Connect a remote node (proxy) to this listening node.
     *
     * The connection descriptions of the node are used to connect the
     * remote local node. On success, the node is in the connected state,
     * otherwise its state is unchanged.
     *
     * This method is one-sided, that is, the node to be connected should
     * not initiate a connection to this node at the same time. For
     * concurrent connects use the other connect() method using node
     * identifiers.
     *
     * @param node the remote node.
     * @return true if this node was connected, false otherwise.
     * @version 1.0
     */
    CO_API bool connect( NodePtr node );

    /**
     * Create and connect a node given by an identifier.
     *
     * This method is two-sided and thread-safe, that is, it can be called
     * by multiple threads on the same node with the same nodeID, or
     * concurrently on two nodes with each others' nodeID.
     *
     * @param nodeID the identifier of the node to connect.
     * @return the connected node, or an invalid RefPtr if the node could
     *         not be connected.
     * @version 1.0
     */
    CO_API NodePtr connect( const NodeID& nodeID );

    /**
     * Find and connect the node where the given object is registered.
     *
     * This method is relatively expensive, since potentially all connected
     * nodes are queried.
     *
     * @param id the identifier of the object to search for.
     * @return the connected node, or an invalid RefPtr if the node could
     *         not be found or connected.
     * @sa registerObject(), connect()
     * @version 1.1.1
     */
    CO_API NodePtr connectObjectMaster( const uint128_t& id );

    /**
     * Disconnect a connected node.
     *
     * @param node the remote node.
     * @return true if the node was disconnected correctly, false otherwise.
     * @version 1.0
     */
    CO_API virtual bool disconnect( NodePtr node );
    //@}

    /** @name Object Registry */
    //@{
    /**
     * Register a distributed object.
     *
     * Registering a distributed object makes this object the master
     * version. The object's identifier is used to map slave instances of
     * the object. Master versions of objects are typically writable and can
     * commit new versions of the distributed object.
     *
     * @param object the object instance.
     * @return true if the object was registered, false otherwise.
     * @version 1.0
     */
    CO_API bool registerObject( Object* object ) override;

    /**
     * Deregister a distributed object.
     *
     * All slave instances should be unmapped before this call, and will be
     * forcefully unmapped by this method.
     *
     * @param object the object instance.
     * @version 1.0
     */
    CO_API void deregisterObject( Object* object ) override;

    /**
     * Map a distributed object.
     *
     * The mapped object becomes a slave instance of the master version which
     * was registered with the provided identifier. The given version can be
     * used to map a specific version.
     *
     * If VERSION_NONE is provided, the slave instance is not initialized with
     * any data from the master. This is useful if the object has been
     * pre-initialized by other means, for example from a shared file system.
     *
     * If VERSION_OLDEST is provided, the oldest available version is mapped.
     *
     * If a concrete requested version no longer exists, mapObject() will map
     * the oldest available version.
     *
     * If the requested version is newer than the head version, mapObject() will
     * block until the requested version is available.
     *
     * Mapping an object is a potentially time-consuming operation. Using
     * mapObjectNB() and mapObjectSync() to asynchronously map multiple objects
     * in parallel improves performance of this operation.
     *
     * After mapping, the object will have the version used during
     * initialization, or VERSION_NONE if mapped to this version.
     *
     * When no master node is given, connectObjectMaster() is used to find the
     * node with the master instance.
     *
     * This method returns immediately after initiating the mapping. Evaluating
     * the value of the returned lunchbox::Future will block on the completion
     * of the operation and return true if the object was mapped, false if the
     * master of the object is not found or the requested version is no longer
     * available.
     *
     * @param object the object.
     * @param id the master object identifier.
     * @param master the node with the master instance, may be 0.
     * @param version the initial version.
     * @return A lunchbox::Future which will deliver the success status of
     *         the operation on evaluation.
     * @sa registerObject
     * @version 1.0
     */
    CO_API f_bool_t mapObject( Object* object, const uint128_t& id,
                               NodePtr master,
                               const uint128_t& version = VERSION_OLDEST );

    /** Convenience wrapper for mapObject(). @version 1.0 */
    f_bool_t mapObject( Object* object, const ObjectVersion& v )
        { return mapObject( object, v.identifier, 0, v.version ); }

    /** @deprecated */
    f_bool_t mapObject( Object* object, const uint128_t& id,
                        const uint128_t& version = VERSION_OLDEST )
        { return mapObject( object, id, 0, version ); }

    /** @deprecated use mapObject() */
    CO_API uint32_t mapObjectNB( Object* object, const uint128_t& id,
                                 const uint128_t& version = VERSION_OLDEST );

    /** @deprecated use mapObject() */
    CO_API uint32_t mapObjectNB( Object* object, const uint128_t& id,
                                 const uint128_t& version,
                                 NodePtr master ) override;

    /** @deprecated use mapObject() */
    CO_API bool mapObjectSync( const uint32_t requestID ) override;

    /**
     * Synchronize the local object with a remote object.
     *
     * The object is synchronized to the newest version of the first
     * attached object on the given master node matching the
     * instanceID. When no master node is given, connectObjectMaster() is
     * used to find the node with the master instance. When CO_INSTANCE_ALL
     * is given, the first instance is used. Before a successful return,
     * applyInstanceData() is called on the calling thread to synchronize
     * the given object.
     *
     * @param object The local object instance to synchronize.
     * @param master The node where the synchronizing object is attached.
     * @param id the object identifier.
     * @param instanceID the instance identifier of the synchronizing
     *                   object.
     * @return A lunchbox::Future which will deliver the success status of
     *         the operation on evaluation.
     * @version 1.1.1
     */
    CO_API f_bool_t syncObject( Object* object, NodePtr master,
                                const uint128_t& id,
                         const uint32_t instanceID = CO_INSTANCE_ALL ) override;
    /**
     * Unmap a mapped object.
     *
     * @param object the mapped object.
     * @version 1.0
     */
    CO_API void unmapObject( Object* object ) override;

    /** Disable the instance cache of a stopped local node. @version 1.0 */
    CO_API void disableInstanceCache();

    /** @internal */
    CO_API void expireInstanceData( const int64_t age );

    /**
     * Enable sending instance data after registration.
     *
     * Send-on-register starts transmitting instance data of registered
     * objects directly after they have been registered. The data is cached
     * on remote nodes and accelerates object mapping. Send-on-register
     * should not be active when remote nodes are joining a multicast group
     * of this node, since they will potentially read out-of-order data
     * streams on the multicast connection.
     *
     * Enable and disable are counted, that is, the last enable on a
     * matched series of disable/enable will be effective. The disable is
     * completely synchronous, that is, no more instance data will be sent
     * after the first disable.
     *
     * @version 1.0
     */
    CO_API void enableSendOnRegister();

    /** Disable sending data of newly registered objects. @version 1.0 */
    CO_API void disableSendOnRegister();

    /**
     * Handler for an Object::push() operation.
     *
     * Called at least on each node listed in an Object::push() operation
     * upon reception of the pushed data from the command thread. Called on
     * all nodes of a multicast group, even for nodes not listed in the
     * Object::push().
     *
     * The default implementation calls registered push handlers. Typically
     * used to create an object on a remote node, using the objectType for
     * instantiation, the istream to initialize it, and the objectID to map
     * it using VERSION_NONE. The groupID may be used to differentiate
     * multiple concurrent push operations.
     *
     * @param groupID The group identifier given to Object::push()
     * @param objectType The type identifier given to Object::push()
     * @param objectID The identifier of the pushed object
     * @param istream the input data stream containing the instance data.
     * @version 1.0
     */
    CO_API virtual void objectPush( const uint128_t& groupID,
                                    const uint128_t& objectType,
                                    const uint128_t& objectID,
                                    DataIStream& istream );

    /** Function signature for push handlers. @version 1.0 */
    typedef boost::function< void( const uint128_t&, //!< groupID
                                   const uint128_t&, //!< objectType
                                   const uint128_t&, //!< objectID
                                   DataIStream& ) > PushHandler;
    /**
     * Register a custom handler for Object::push operations
     *
     * The registered handler function will be called automatically for an
     * incoming object push. Threadsafe with itself and objectPush().
     *
     * @param groupID The group identifier given to Object::push()
     * @param handler The handler function called for a registered groupID
     * @version 1.0
     */
    CO_API void registerPushHandler( const uint128_t& groupID,
                                     const PushHandler& handler );


    /** Function signature for custom command handlers. @version 1.0 */
    typedef boost::function< bool( CustomICommand& ) > CommandHandler;

    /**
     * Register a custom command handler handled by this node.
     *
     * Custom command handlers are invoked on reception of a CustomICommand
     * send by Node::send( uint128_t, ... ). The command identifier needs to
     * be unique. It is recommended to use an UUID or
     * lunchbox::make_uint128() to generate this identifier.
     *
     * @param command the unique identifier of the custom command
     * @param func the handler function for the custom command
     * @param queue the queue where the command should be inserted to
     * @return true on successful registering, false otherwise
     * @version 1.0
     */
    CO_API bool registerCommandHandler( const uint128_t& command,
                                        const CommandHandler& func,
                                        CommandQueue* queue );

    /** @internal swap the existing object by a new object and keep
        the cm, id and instanceID. */
    CO_API void swapObject( Object* oldObject, Object* newObject );
    //@}

    /** @name Data Access */
    //@{
    /**
     * Get a node by identifier.
     *
     * The node might not be connected. Thread safe.
     *
     * @param id the node identifier.
     * @return the node.
     * @version 1.0
     */
    CO_API NodePtr getNode( const NodeID& id ) const;

    /** Assemble a vector of the currently connected nodes. @version 1.0 */
    CO_API void getNodes( Nodes& nodes, const bool addSelf = true ) const;

    /** Return the command queue to the command thread. @version 1.0 */
    CO_API CommandQueue* getCommandThreadQueue();

    /**
     * @return true if executed from the command handler thread, false if
     *         not.
     * @version 1.0
     */
    CO_API bool inCommandThread() const;

    CO_API int64_t getTime64() const; //!< @internal
    CO_API ssize_t getCounter( const Counter counter ) const; //!< @internal
    //@}

    /** @name Operations */
    //@{
    /**
     * Add a listening connection to this listening node.
     * @return the listening connection, or 0 upon error.
     */
    CO_API ConnectionPtr addListener( ConnectionDescriptionPtr desc );

    /** Add a listening connection to this listening node. */
    CO_API void addListener( ConnectionPtr connection );

    /** Remove listening connections from this listening node.*/
    CO_API void removeListeners( const Connections& connections );

    /** @internal
     * Flush all pending commands on this listening node.
     *
     * This causes the receiver thread to redispatch all pending commands,
     * which are normally only redispatched when a new command is received.
     */
    CO_API void flushCommands();

    /** @internal Allocate a command buffer from the receiver thread. */
    CO_API BufferPtr allocBuffer( const uint64_t size );

    /**
     * Dispatches a command to the registered command queue.
     *
     * Applications using custom command types have to override this method
     * to dispatch the custom commands.
     *
     * @param command the command.
     * @return the result of the operation.
     * @sa ICommand::invoke
     * @version 1.0
     */
    CO_API bool dispatchCommand( ICommand& command ) override;


    /** A handle for a send token acquired by acquireSendToken(). */
    typedef lunchbox::RefPtr< co::SendToken > SendToken;

    /**
     * Acquire a send token from the given node.
     *
     * The token is released automatically when it leaves its scope or
     * explicitly using releaseSendToken().
     *
     * @return The send token.
     */
    CO_API SendToken acquireSendToken( NodePtr toNode );

    /** @deprecated Token will auto-release when leaving scope. */
    CO_API void releaseSendToken( SendToken token );

    /** @return a Zeroconf communicator handle for this node. @version 1.0*/
    CO_API Zeroconf getZeroconf();
    //@}

    /** @internal Ack an operation to the sender. */
    CO_API void ackRequest( NodePtr node, const uint32_t requestID );

    /** Request keep-alive update from the remote node. */
    CO_API void ping( NodePtr remoteNode );

    /**
     * Request updates from all nodes above keep-alive timeout.
     *
     * @return true if at least one ping was send.
     */
    CO_API bool pingIdleNodes();

    /**
     * Bind this, the receiver and the command thread to the given
     * lunchbox::Thread affinity.
     */
    CO_API void setAffinity( const int32_t affinity );

    /** @internal */
    CO_API void addConnection( ConnectionPtr connection );

protected:
    /** Destruct this local node. @version 1.0 */
    CO_API ~LocalNode() override;

    /** @internal
     * Connect a node proxy to this node.
     *
     * This node has to be in the listening state. The node proxy will be
     * put in the connected state upon success. The connection has to be
     * connected.
     *
     * @param node the remote node.
     * @param connection the connection to the remote node.
     * @return true if the node was connected correctly,
     *         false otherwise.
     */
    CO_API bool connect( NodePtr node, ConnectionPtr connection );

    /** @internal Notify remote node connection. */
    virtual void notifyConnect( NodePtr ) {}

    /** @internal Notify remote node disconnection. */
    virtual void notifyDisconnect( NodePtr ) {}

    /**
     * Factory method to create a new node.
     *
     * @param type the type the node type
     * @return the node.
     * @sa ctor type parameter
     * @version 1.0
     */
    CO_API virtual NodePtr createNode( const uint32_t type );

private:
    detail::LocalNode* const _impl;

    friend class detail::ReceiverThread;
    bool _startCommandThread( const int32_t threadID );
    void _runReceiverThread();

    friend class detail::CommandThread;
    bool _notifyCommandThreadIdle();

    void _cleanup();
    void _closeNode( NodePtr node );
    void _addConnection( ConnectionPtr connection );
    void _removeConnection( ConnectionPtr connection );

    lunchbox::Request< void > _removeListener( ConnectionPtr connection );

    uint32_t _connect( NodePtr node );
    NodePtr _connect( const NodeID& nodeID );
    uint32_t _connect( NodePtr node, ConnectionPtr connection );
    NodePtr _connect( const NodeID& nodeID, NodePtr peer );
    NodePtr _connectFromZeroconf( const NodeID& nodeID );
    bool _connectSelf();

    void _handleConnect();
    void _handleDisconnect();
    bool _handleData();
    BufferPtr _readHead( ConnectionPtr connection );
    ICommand _setupCommand( ConnectionPtr, ConstBufferPtr );
    bool _readTail( ICommand&, BufferPtr, ConnectionPtr );
    void _initService();
    void _exitService();

    friend class ObjectStore;
    template< typename T >
    void _registerCommand( const uint32_t command, const CommandFunc< T >& func,
                           CommandQueue* destinationQueue )
    {
        registerCommand( command, func, destinationQueue );
    }

    void _dispatchCommand( ICommand& command );
    void _redispatchCommands();

    /** The command functions. */
    bool _cmdAckRequest( ICommand& command );
    bool _cmdStopRcv( ICommand& command );
    bool _cmdStopCmd( ICommand& command );
    bool _cmdSetAffinity( ICommand& command );
    bool _cmdConnect( ICommand& command );
    bool _cmdConnectReply( ICommand& command );
    bool _cmdConnectAck( ICommand& command );
    bool _cmdID( ICommand& command );
    bool _cmdDisconnect( ICommand& command );
    bool _cmdGetNodeData( ICommand& command );
    bool _cmdGetNodeDataReply( ICommand& command );
    bool _cmdAcquireSendToken( ICommand& command );
    bool _cmdAcquireSendTokenReply( ICommand& command );
    bool _cmdReleaseSendToken( ICommand& command );
    bool _cmdAddListener( ICommand& command );
    bool _cmdRemoveListener( ICommand& command );
    bool _cmdPing( ICommand& command );
    bool _cmdCommand( ICommand& command );
    bool _cmdCommandAsync( ICommand& command );
    bool _cmdAddConnection( ICommand& command );
    bool _cmdDiscard( ICommand& ) { return true; }
    //@}

    LB_TS_VAR( _cmdThread )
    LB_TS_VAR( _rcvThread )
};

inline std::ostream& operator << ( std::ostream& os, const LocalNode& node )
{
    os << static_cast< const Node& >( node );
    return os;
}
}
#endif // CO_LOCALNODE_H
