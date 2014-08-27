
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

#include "localNode.h"

#include "buffer.h"
#include "bufferCache.h"
#include "commandQueue.h"
#include "connectionDescription.h"
#include "connectionSet.h"
#include "customICommand.h"
#include "dataIStream.h"
#include "exception.h"
#include "global.h"
#include "iCommand.h"
#include "nodeCommand.h"
#include "oCommand.h"
#include "object.h"
#include "objectICommand.h"
#include "objectStore.h"
#include "pipeConnection.h"
#include "sendToken.h"
#include "worker.h"
#include "zeroconf.h"

#include <lunchbox/clock.h>
#include <lunchbox/futureFunction.h>
#include <lunchbox/hash.h>
#include <lunchbox/lockable.h>
#include <lunchbox/request.h>
#include <lunchbox/rng.h>
#include <lunchbox/servus.h>
#include <lunchbox/sleep.h>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#include <list>

namespace bp = boost::posix_time;

namespace co
{
namespace
{
lunchbox::a_int32_t _threadIDs;

typedef CommandFunc< LocalNode > CmdFunc;
typedef std::list< ICommand > CommandList;
typedef lunchbox::RefPtrHash< Connection, NodePtr > ConnectionNodeHash;
typedef ConnectionNodeHash::const_iterator ConnectionNodeHashCIter;
typedef ConnectionNodeHash::iterator ConnectionNodeHashIter;
typedef stde::hash_map< uint128_t, NodePtr > NodeHash;
typedef NodeHash::const_iterator NodeHashCIter;
typedef stde::hash_map< uint128_t, LocalNode::PushHandler > HandlerHash;
typedef HandlerHash::const_iterator HandlerHashCIter;
typedef std::pair< LocalNode::CommandHandler, CommandQueue* > CommandPair;
typedef stde::hash_map< uint128_t, CommandPair > CommandHash;
typedef CommandHash::const_iterator CommandHashCIter;
typedef lunchbox::FutureFunction< bool > FuturebImpl;
}

namespace detail
{
class ReceiverThread : public lunchbox::Thread
{
public:
    ReceiverThread( co::LocalNode* localNode ) : _localNode( localNode ) {}
    bool init() override
    {
        const int32_t threadID = ++_threadIDs - 1;
        setName( std::string( "Rcv" ) +
                 boost::lexical_cast< std::string >( threadID ));
        return _localNode->_startCommandThread( threadID );
    }

    void run() override { _localNode->_runReceiverThread(); }

private:
    co::LocalNode* const _localNode;
};

class CommandThread : public Worker
{
public:
    CommandThread( co::LocalNode* localNode )
        : Worker( Global::getCommandQueueLimit( ))
        , threadID( 0 )
        , _localNode( localNode )
    {}

    int32_t threadID;

protected:
    bool init() override
    {
        setName( std::string( "Cmd" ) +
                 boost::lexical_cast< std::string >( threadID ));
        return true;
    }

    bool stopRunning() override { return _localNode->isClosed(); }
    bool notifyIdle() override { return _localNode->_notifyCommandThreadIdle();}

private:
    co::LocalNode* const _localNode;
};

class LocalNode
{
public:
    LocalNode()
        : smallBuffers( 200 )
        , bigBuffers( 20 )
        , sendToken( true )
        , lastSendToken( 0 )
        , objectStore( 0 )
        , receiverThread( 0 )
        , commandThread( 0 )
        , service( "_collage._tcp" )
    {}

    ~LocalNode()
    {
        LBASSERT( incoming.isEmpty( ));
        LBASSERT( connectionNodes.empty( ));
        LBASSERT( pendingCommands.empty( ));
        LBASSERT( nodes->empty( ));

        delete objectStore;
        objectStore = 0;
        LBASSERT( !commandThread->isRunning( ));
        delete commandThread;
        commandThread = 0;

        LBASSERT( !receiverThread->isRunning( ));
        delete receiverThread;
        receiverThread = 0;
    }

    bool inReceiverThread() const { return receiverThread->isCurrent(); }

    /** Commands re-scheduled for dispatch. */
    CommandList pendingCommands;

    /** The command buffer 'allocator' for small packets */
    co::BufferCache smallBuffers;

    /** The command buffer 'allocator' for big packets */
    co::BufferCache bigBuffers;

    bool sendToken; //!< send token availability.
    uint64_t lastSendToken; //!< last used time for timeout detection
    std::deque< co::ICommand > sendTokenQueue; //!< pending requests

    /** Manager of distributed object */
    ObjectStore* objectStore;

    /** Needed for thread-safety during nodeID-based connect() */
    lunchbox::Lock connectLock;

    /** The node for each connection. */
    ConnectionNodeHash connectionNodes; // read and write: recv only

    /** The connected nodes. */
    lunchbox::Lockable< NodeHash, lunchbox::SpinLock > nodes; // r: all, w: recv

    /** The connection set of all connections from/to this node. */
    co::ConnectionSet incoming;

    /** The process-global clock. */
    lunchbox::Clock clock;

    /** The registered push handlers. */
    lunchbox::Lockable< HandlerHash, lunchbox::Lock > pushHandlers;

    /** The registered custom command handlers. */
    lunchbox::Lockable< CommandHash, lunchbox::SpinLock > commandHandlers;

    ReceiverThread* receiverThread;
    CommandThread* commandThread;

    lunchbox::Lockable< lunchbox::Servus > service;

    // Performance counters:
    a_ssize_t counters[ co::LocalNode::COUNTER_ALL ];
};
}

LocalNode::LocalNode( const uint32_t type )
        : Node( type )
        , _impl( new detail::LocalNode )
{
    _impl->receiverThread = new detail::ReceiverThread( this );
    _impl->commandThread  = new detail::CommandThread( this );
    _impl->objectStore = new ObjectStore( this, _impl->counters );

    CommandQueue* queue = getCommandThreadQueue();
    registerCommand( CMD_NODE_CONNECT,
                     CmdFunc( this, &LocalNode::_cmdConnect ), 0 );
    registerCommand( CMD_NODE_CONNECT_BE,
                     CmdFunc( this, &LocalNode::_cmdConnect ), 0 );
    registerCommand( CMD_NODE_CONNECT_REPLY,
                     CmdFunc( this, &LocalNode::_cmdConnectReply ), 0 );
    registerCommand( CMD_NODE_CONNECT_REPLY_BE,
                     CmdFunc( this, &LocalNode::_cmdConnectReply ), 0 );
    registerCommand( CMD_NODE_ID,
                     CmdFunc( this, &LocalNode::_cmdID ), 0 );
    registerCommand( CMD_NODE_ID_BE,
                     CmdFunc( this, &LocalNode::_cmdID ), 0 );
    registerCommand( CMD_NODE_ACK_REQUEST,
                     CmdFunc( this, &LocalNode::_cmdAckRequest ), 0 );
    registerCommand( CMD_NODE_STOP_RCV,
                     CmdFunc( this, &LocalNode::_cmdStopRcv ), 0 );
    registerCommand( CMD_NODE_STOP_CMD,
                     CmdFunc( this, &LocalNode::_cmdStopCmd ), queue );
    registerCommand( CMD_NODE_SET_AFFINITY_RCV,
                     CmdFunc( this, &LocalNode::_cmdSetAffinity ), 0 );
    registerCommand( CMD_NODE_SET_AFFINITY_CMD,
                     CmdFunc( this, &LocalNode::_cmdSetAffinity ), queue );
    registerCommand( CMD_NODE_CONNECT_ACK,
                     CmdFunc( this, &LocalNode::_cmdConnectAck ), 0 );
    registerCommand( CMD_NODE_DISCONNECT,
                     CmdFunc( this, &LocalNode::_cmdDisconnect ), 0 );
    registerCommand( CMD_NODE_GET_NODE_DATA,
                     CmdFunc( this, &LocalNode::_cmdGetNodeData ), queue );
    registerCommand( CMD_NODE_GET_NODE_DATA_REPLY,
                     CmdFunc( this, &LocalNode::_cmdGetNodeDataReply ), 0 );
    registerCommand( CMD_NODE_ACQUIRE_SEND_TOKEN,
                     CmdFunc( this, &LocalNode::_cmdAcquireSendToken ), queue );
    registerCommand( CMD_NODE_ACQUIRE_SEND_TOKEN_REPLY,
                     CmdFunc( this, &LocalNode::_cmdAcquireSendTokenReply ), 0);
    registerCommand( CMD_NODE_RELEASE_SEND_TOKEN,
                     CmdFunc( this, &LocalNode::_cmdReleaseSendToken ), queue );
    registerCommand( CMD_NODE_ADD_LISTENER,
                     CmdFunc( this, &LocalNode::_cmdAddListener ), 0 );
    registerCommand( CMD_NODE_REMOVE_LISTENER,
                     CmdFunc( this, &LocalNode::_cmdRemoveListener ), 0 );
    registerCommand( CMD_NODE_PING,
                     CmdFunc( this, &LocalNode::_cmdPing ), queue );
    registerCommand( CMD_NODE_PING_REPLY,
                     CmdFunc( this, &LocalNode::_cmdDiscard ), 0 );
    registerCommand( CMD_NODE_COMMAND,
                     CmdFunc( this, &LocalNode::_cmdCommand ), 0 );
    registerCommand( CMD_NODE_ADD_CONNECTION,
                     CmdFunc( this, &LocalNode::_cmdAddConnection ), 0 );
}

LocalNode::~LocalNode( )
{
    LBASSERT( !hasPendingRequests( ));
    delete _impl;
}

bool LocalNode::initLocal( const int argc, char** argv )
{
#ifndef NDEBUG
    LBVERB << lunchbox::disableFlush << "args: ";
    for( int i=0; i<argc; i++ )
         LBVERB << argv[i] << ", ";
    LBVERB << std::endl << lunchbox::enableFlush;
#endif

    // We do not use getopt_long because it really does not work due to the
    // following aspects:
    // - reordering of arguments
    // - different behavior of GNU and BSD implementations
    // - incomplete man pages
    for( int i=1; i<argc; ++i )
    {
        if( std::string( "--eq-listen" ) == argv[i] )
            LBWARN << "Deprecated --eq-listen, use --co-listen" << std::endl;
        if( std::string( "--eq-listen" ) == argv[i] ||
            std::string( "--co-listen" ) == argv[i] )
        {
            if( (i+1)<argc && argv[i+1][0] != '-' )
            {
                std::string data = argv[++i];
                ConnectionDescriptionPtr desc = new ConnectionDescription;
                desc->port = Global::getDefaultPort();

                if( desc->fromString( data ))
                {
                    addConnectionDescription( desc );
                    LBASSERTINFO( data.empty(), data );
                }
                else
                    LBWARN << "Ignoring listen option: " << argv[i] <<std::endl;
            }
            else
            {
                LBWARN << "No argument given to --co-listen!" << std::endl;
            }
        }
        else if ( std::string( "--co-globals" ) == argv[i] )
        {
            if( (i+1)<argc && argv[i+1][0] != '-' )
            {
                const std::string data = argv[++i];
                if( !Global::fromString( data ))
                {
                    LBWARN << "Invalid global variables string: " << data
                           << ", using default global variables." << std::endl;
                }
            }
            else
            {
                LBWARN << "No argument given to --co-globals!" << std::endl;
            }
        }
    }

    if( !listen( ))
    {
        LBWARN << "Can't setup listener(s) on " << *static_cast< Node* >( this )
               << std::endl;
        return false;
    }
    return true;
}

bool LocalNode::listen()
{
    LBVERB << "Listener data: " << serialize() << std::endl;
    if( !isClosed() || !_connectSelf( ))
        return false;

    const ConnectionDescriptions& descriptions = getConnectionDescriptions();
    for( ConnectionDescriptionsCIter i = descriptions.begin();
         i != descriptions.end(); ++i )
    {
        ConnectionDescriptionPtr description = *i;
        ConnectionPtr connection = Connection::create( description );

        if( !connection || !connection->listen( ))
        {
            LBWARN << "Can't create listener connection: " << description
                   << std::endl;
            return false;
        }

        _impl->connectionNodes[ connection ] = this;
        if( connection->isMulticast( ))
            _addMulticast( this, connection );

        connection->acceptNB();
        _impl->incoming.addConnection( connection );

        LBVERB << "Added node " << getNodeID() << " using " << connection
               << std::endl;
    }

    LBVERB << lunchbox::className(this) << " start command and receiver thread "
           << std::endl;

    _setListening();
    _impl->receiverThread->start();

    LBINFO << *this << std::endl;
    return true;
}

bool LocalNode::listen( ConnectionPtr connection )
{
    if( !listen( ))
        return false;
    _addConnection( connection );
    return true;
}

bool LocalNode::close()
{
    if( !isListening() )
        return false;

    send( CMD_NODE_STOP_RCV );

    LBCHECK( _impl->receiverThread->join( ));
    _cleanup();

    LBINFO << _impl->incoming.getSize() << " connections open after close"
           << std::endl;
#ifndef NDEBUG
    const Connections& connections = _impl->incoming.getConnections();
    for( Connections::const_iterator i = connections.begin();
         i != connections.end(); ++i )
    {
        LBINFO << "    " << *i << std::endl;
    }
#endif

    LBASSERTINFO( !hasPendingRequests(),
                  *static_cast< lunchbox::RequestHandler* >( this ));
    return true;
}

void LocalNode::setAffinity( const int32_t affinity )
{
    send( CMD_NODE_SET_AFFINITY_RCV ) << affinity;
    send( CMD_NODE_SET_AFFINITY_CMD ) << affinity;

    lunchbox::Thread::setAffinity( affinity );
}

ConnectionPtr LocalNode::addListener( ConnectionDescriptionPtr desc )
{
    LBASSERT( isListening( ));

    ConnectionPtr connection = Connection::create( desc );
    if( connection && connection->listen( ))
    {
        addListener( connection );
        return connection;
    }

    return 0;
}

void LocalNode::addListener( ConnectionPtr connection )
{
    LBASSERT( isListening( ));
    LBASSERT( connection->isListening( ));
    if( !isListening() || !connection->isListening( ))
        return;

    connection->ref( this ); // unref in self handler

    // Update everybody's description list of me, add the listener to myself in
    // my handler
    Nodes nodes;
    getNodes( nodes );

    for( NodesIter i = nodes.begin(); i != nodes.end(); ++i )
        (*i)->send( CMD_NODE_ADD_LISTENER )
            << (uint64_t)(connection.get( ))
            << connection->getDescription()->toString();
}

void LocalNode::removeListeners( const Connections& connections )
{
    std::vector< lunchbox::Request< void > > requests;
    for( ConnectionsCIter i = connections.begin(); i != connections.end(); ++i )
    {
        ConnectionPtr connection = *i;
        requests.push_back( _removeListener( connection ));
    }
}

lunchbox::Request< void > LocalNode::_removeListener( ConnectionPtr conn )
{
    LBASSERT( isListening( ));
    LBASSERTINFO( !conn->isConnected(), conn );

    conn->ref( this );
    const lunchbox::Request< void > request = registerRequest< void >();
    Nodes nodes;
    getNodes( nodes );

    for( NodesIter i = nodes.begin(); i != nodes.end(); ++i )
        (*i)->send( CMD_NODE_REMOVE_LISTENER ) << request << conn.get()
                                          << conn->getDescription()->toString();
    return request;
}

void LocalNode::_addConnection( ConnectionPtr connection )
{
    if( _impl->receiverThread->isRunning() && !_impl->inReceiverThread( ))
    {
        connection->ref(); // unref in _cmdAddConnection
        send( CMD_NODE_ADD_CONNECTION ) << connection;
        return;
    }

    BufferPtr buffer = _impl->smallBuffers.alloc( COMMAND_ALLOCSIZE );
    connection->recvNB( buffer, COMMAND_MINSIZE );
    _impl->incoming.addConnection( connection );
}

void LocalNode::_removeConnection( ConnectionPtr connection )
{
    LBASSERT( connection );

    _impl->incoming.removeConnection( connection );
    connection->resetRecvData();
    if( !connection->isClosed( ))
        connection->close(); // cancel pending IO's
}

void LocalNode::_cleanup()
{
    LBVERB << "Clean up stopped node" << std::endl;
    LBASSERTINFO( isClosed(), *this );

    if( !_impl->connectionNodes.empty( ))
        LBINFO << _impl->connectionNodes.size()
               << " open connections during cleanup" << std::endl;
#ifndef NDEBUG
    for( ConnectionNodeHashCIter i = _impl->connectionNodes.begin();
         i != _impl->connectionNodes.end(); ++i )
    {
        NodePtr node = i->second;
        LBINFO << "    " << i->first << " : " << node << std::endl;
    }
#endif

    _impl->connectionNodes.clear();

    if( !_impl->nodes->empty( ))
        LBINFO << _impl->nodes->size() << " nodes connected during cleanup"
               << std::endl;

#ifndef NDEBUG
    for( NodeHashCIter i = _impl->nodes->begin(); i != _impl->nodes->end(); ++i)
    {
        NodePtr node = i->second;
        LBINFO << "    " << node << std::endl;
    }
#endif

    _impl->nodes->clear();
}

void LocalNode::_closeNode( NodePtr node )
{
    ConnectionPtr connection = node->getConnection();
    ConnectionPtr mcConnection = node->_getMulticast();

    node->_disconnect();

    if( connection )
    {
        LBASSERTINFO( _impl->connectionNodes.find( connection ) !=
                      _impl->connectionNodes.end(), connection );

        _removeConnection( connection );
        _impl->connectionNodes.erase( connection );
    }

    if( mcConnection )
    {
        _removeConnection( mcConnection );
        _impl->connectionNodes.erase( mcConnection );
    }

    _impl->objectStore->removeInstanceData( node->getNodeID( ));

    lunchbox::ScopedFastWrite mutex( _impl->nodes );
    _impl->nodes->erase( node->getNodeID( ));
    notifyDisconnect( node );
    LBINFO << node << " disconnected from " << *this << std::endl;
}

bool LocalNode::_connectSelf()
{
    // setup local connection to myself
    PipeConnectionPtr connection = new PipeConnection;
    if( !connection->connect( ))
    {
        LBERROR << "Could not create local connection to receiver thread."
                << std::endl;
        return false;
    }

    Node::_connect( connection->getSibling( ));
    _setClosed(); // reset state after _connect set it to connected

    // add to connection set
    LBASSERT( connection->getDescription( ));
    LBASSERT( _impl->connectionNodes.find( connection ) ==
              _impl->connectionNodes.end( ));

    _impl->connectionNodes[ connection ] = this;
    _impl->nodes.data[ getNodeID() ] = this;
    _addConnection( connection );

    LBVERB << "Added node " << getNodeID() << " using " << connection
           << std::endl;
    return true;
}

bool LocalNode::disconnect( NodePtr node )
{
    if( !node || !isListening() )
        return false;

    if( !node->isConnected( ))
        return true;

    LBASSERT( !inCommandThread( ));
    lunchbox::Request< void > request = registerRequest< void >( node.get( ));
    send( CMD_NODE_DISCONNECT ) << request;

    request.wait();
    _impl->objectStore->removeNode( node );
    return true;
}

void LocalNode::ackRequest( NodePtr node, const uint32_t requestID )
{
    if( requestID == LB_UNDEFINED_UINT32 ) // no need to ack operation
        return;

    if( node == this ) // OPT
        serveRequest( requestID );
    else
        node->send( CMD_NODE_ACK_REQUEST ) << requestID;
}

void LocalNode::ping( NodePtr peer )
{
    LBASSERT( !_impl->inReceiverThread( ));
    peer->send( CMD_NODE_PING );
}

bool LocalNode::pingIdleNodes()
{
    LBASSERT( !_impl->inReceiverThread( ) );
    const int64_t timeout = Global::getKeepaliveTimeout() / 2;
    Nodes nodes;
    getNodes( nodes, false );

    bool pinged = false;
    for( NodesCIter i = nodes.begin(); i != nodes.end(); ++i )
    {
        NodePtr node = *i;
        if( getTime64() - node->getLastReceiveTime() > timeout )
        {
            LBINFO << " Ping Node: " <<  node->getNodeID() << " last seen "
                   << node->getLastReceiveTime() << std::endl;
            node->send( CMD_NODE_PING );
            pinged = true;
        }
    }
    return pinged;
}

//----------------------------------------------------------------------
// Object functionality
//----------------------------------------------------------------------
void LocalNode::disableInstanceCache()
{
    _impl->objectStore->disableInstanceCache();
}

void LocalNode::expireInstanceData( const int64_t age )
{
    _impl->objectStore->expireInstanceData( age );
}

void LocalNode::enableSendOnRegister()
{
    _impl->objectStore->enableSendOnRegister();
}

void LocalNode::disableSendOnRegister()
{
    _impl->objectStore->disableSendOnRegister();
}

bool LocalNode::registerObject( Object* object )
{
    return _impl->objectStore->register_( object );
}

void LocalNode::deregisterObject( Object* object )
{
    _impl->objectStore->deregister( object );
}

f_bool_t LocalNode::mapObject( Object* object, const uint128_t& id,
                               NodePtr master, const uint128_t& version )
{
    const uint32_t request = _impl->objectStore->mapNB( object, id, version,
                                                        master );
    const FuturebImpl::Func& func = boost::bind( &ObjectStore::mapSync,
                                                 _impl->objectStore, request );
    return f_bool_t( new FuturebImpl( func ));
}

uint32_t LocalNode::mapObjectNB( Object* object, const uint128_t& id,
                                 const uint128_t& version )
{
    return _impl->objectStore->mapNB( object, id, version, 0 );
}

uint32_t LocalNode::mapObjectNB( Object* object, const uint128_t& id,
                                 const uint128_t& version, NodePtr master )
{
    return _impl->objectStore->mapNB( object, id, version, master );
}


bool LocalNode::mapObjectSync( const uint32_t requestID )
{
    return _impl->objectStore->mapSync( requestID );
}

f_bool_t LocalNode::syncObject( Object* object, NodePtr master, const uint128_t& id,
                               const uint32_t instanceID )
{
    return _impl->objectStore->sync( object, master, id, instanceID );
}

void LocalNode::unmapObject( Object* object )
{
    _impl->objectStore->unmap( object );
}

void LocalNode::swapObject( Object* oldObject, Object* newObject )
{
    _impl->objectStore->swap( oldObject, newObject );
}

void LocalNode::objectPush( const uint128_t& groupID,
                            const uint128_t& objectType,
                            const uint128_t& objectID, DataIStream& istream )
{
    lunchbox::ScopedRead mutex( _impl->pushHandlers );
    HandlerHashCIter i = _impl->pushHandlers->find( groupID );
    if( i != _impl->pushHandlers->end( ))
        i->second( groupID, objectType, objectID, istream );
    else
        LBWARN << "No custom handler for push group " << groupID
               << " registered" << std::endl;

    if( istream.wasUsed() && istream.hasData( ))
        LBWARN << "Incomplete Object::push for group " << groupID << " type "
               << objectType << " object " << objectID << std::endl;
}

void LocalNode::registerPushHandler( const uint128_t& groupID,
                                     const PushHandler& handler )
{
    lunchbox::ScopedWrite mutex( _impl->pushHandlers );
    (*_impl->pushHandlers)[ groupID ] = handler;
}

bool LocalNode::registerCommandHandler( const uint128_t& command,
                                        const CommandHandler& func,
                                        CommandQueue* queue )
{
    lunchbox::ScopedFastWrite mutex( _impl->commandHandlers );
    if( _impl->commandHandlers->find(command) != _impl->commandHandlers->end( ))
    {
        LBWARN << "Already got a registered handler for custom command "
               << command << std::endl;
        return false;
    }

    _impl->commandHandlers->insert( std::make_pair( command,
                                                std::make_pair( func, queue )));
    return true;
}

LocalNode::SendToken LocalNode::acquireSendToken( NodePtr node )
{
    LBASSERT( !inCommandThread( ));
    LBASSERT( !_impl->inReceiverThread( ));

    lunchbox::Request< void > request = registerRequest< void >();
    node->send( CMD_NODE_ACQUIRE_SEND_TOKEN ) << request;

    try
    {
        request.wait(  Global::getTimeout() );
    }
    catch ( lunchbox::FutureTimeout& )
    {
        LBERROR << "Timeout while acquiring send token " << request.getID()
                << std::endl;
        request.relinquish();
        return 0;
    }
    return new co::SendToken( node );
}

void LocalNode::releaseSendToken( SendToken token )
{
    LBASSERT( !_impl->inReceiverThread( ));
    if( token )
        token->release();
}

//----------------------------------------------------------------------
// Connecting a node
//----------------------------------------------------------------------
namespace
{
enum ConnectResult
{
    CONNECT_OK,
    CONNECT_TRY_AGAIN,
    CONNECT_BAD_STATE,
    CONNECT_TIMEOUT,
    CONNECT_UNREACHABLE
};
}

NodePtr LocalNode::connect( const NodeID& nodeID )
{
    LBASSERT( nodeID != 0 );
    LBASSERT( isListening( ));

    // Make sure that only one connection request based on the node identifier
    // is pending at a given time. Otherwise a node with the same id might be
    // instantiated twice in _cmdGetNodeDataReply(). The alternative to this
    // mutex is to register connecting nodes with this local node, and handle
    // all cases correctly, which is far more complex. Node connections only
    // happen a lot during initialization, and are therefore not time-critical.
    lunchbox::ScopedWrite mutex( _impl->connectLock );

    Nodes nodes;
    getNodes( nodes );

    for( NodesCIter i = nodes.begin(); i != nodes.end(); ++i )
    {
        NodePtr peer = *i;
        if( peer->getNodeID() == nodeID && peer->isReachable( )) // early out
            return peer;
    }

    LBINFO << "Connecting node " << nodeID << std::endl;
    for( NodesCIter i = nodes.begin(); i != nodes.end(); ++i )
    {
        NodePtr peer = *i;
        NodePtr node = _connect( nodeID, peer );
        if( node )
            return node;
    }

    NodePtr node = _connectFromZeroconf( nodeID );
    if( node )
        return node;

    // check again if node connected by itself by now
    nodes.clear();
    getNodes( nodes );
    for( NodesCIter i = nodes.begin(); i != nodes.end(); ++i )
    {
        node = *i;
        if( node->getNodeID() == nodeID && node->isReachable( ))
            return node;
    }

    LBWARN << "Node " << nodeID << " connection failed" << std::endl;
    return 0;
}

NodePtr LocalNode::_connect( const NodeID& nodeID, NodePtr peer )
{
    LBASSERT( nodeID != 0 );

    NodePtr node;
    {
        lunchbox::ScopedFastRead mutexNodes( _impl->nodes );
        NodeHash::const_iterator i = _impl->nodes->find( nodeID );
        if( i != _impl->nodes->end( ))
            node = i->second;
    }

    LBASSERT( getNodeID() != nodeID );
    if( !node )
    {
        lunchbox::Request< void* > request = registerRequest< void* >();
        peer->send( CMD_NODE_GET_NODE_DATA ) << nodeID << request;
        node = reinterpret_cast< Node* >( request.wait( ));
        if( !node )
        {
            LBINFO << "Node " << nodeID << " not found on " << peer->getNodeID()
                   << std::endl;
            return 0;
        }
        node->unref( this ); // ref'd before serveRequest()
    }

    if( node->isReachable( ))
        return node;

    size_t tries = 10;
    while( --tries )
    {
        switch( _connect( node ))
        {
          case CONNECT_OK:
              return node;
          case CONNECT_TRY_AGAIN:
          {
              lunchbox::RNG rng;
              // collision avoidance
              lunchbox::sleep( rng.get< uint8_t >( ));
              break;
          }
          case CONNECT_BAD_STATE:
              LBWARN << "Internal connect error" << std::endl;
              // no break;
          case CONNECT_TIMEOUT:
              return 0;

          case CONNECT_UNREACHABLE:
              break; // maybe peer talks to us
        }

        lunchbox::ScopedFastRead mutexNodes( _impl->nodes );
        // connect failed - check for simultaneous connect from peer
        NodeHash::const_iterator i = _impl->nodes->find( nodeID );
        if( i != _impl->nodes->end( ))
            node = i->second;
    }

    return node->isReachable() ? node : 0;
}

NodePtr LocalNode::_connectFromZeroconf( const NodeID& nodeID )
{
    lunchbox::ScopedWrite mutex( _impl->service );

    const Strings& instances =
        _impl->service->discover( lunchbox::Servus::IF_ALL, 500 );
    for( StringsCIter i = instances.begin(); i != instances.end(); ++i )
    {
        const std::string& instance = *i;
        const NodeID candidate( instance );
        if( candidate != nodeID )
            continue;

        const std::string& typeStr = _impl->service->get( instance, "co_type" );
        if( typeStr.empty( ))
            return 0;

        std::istringstream in( typeStr );
        uint32_t type = 0;
        in >> type;

        NodePtr node = createNode( type );
        if( !node )
        {
            LBINFO << "Can't create node of type " << type << std::endl;
            continue;
        }

        const std::string& numStr = _impl->service->get( instance,
                                                         "co_numPorts" );
        uint32_t num = 0;

        in.clear();
        in.str( numStr );
        in >> num;
        LBASSERT( num > 0 );
        for( size_t j = 0; j < num; ++j )
        {
            ConnectionDescriptionPtr desc = new ConnectionDescription;
            std::ostringstream out;
            out << "co_port" << j;

            std::string descStr = _impl->service->get( instance, out.str( ));
            LBASSERT( !descStr.empty( ));
            LBCHECK( desc->fromString( descStr ));
            LBASSERT( descStr.empty( ));
            node->addConnectionDescription( desc );
        }
        mutex.leave();
        if( _connect( node ))
            return node;
    }
    return 0;
}

bool LocalNode::connect( NodePtr node )
{
    lunchbox::ScopedWrite mutex( _impl->connectLock );
    return ( _connect( node ) == CONNECT_OK );
}

uint32_t LocalNode::_connect( NodePtr node )
{
    LBASSERTINFO( isListening(), *this );
    if( node->isReachable( ))
        return CONNECT_OK;

    LBASSERT( node->isClosed( ));
    LBINFO << "Connecting " << node << std::endl;

    // try connecting using the given descriptions
    const ConnectionDescriptions& cds = node->getConnectionDescriptions();
    for( ConnectionDescriptionsCIter i = cds.begin();
        i != cds.end(); ++i )
    {
        ConnectionDescriptionPtr description = *i;
        if( description->type >= CONNECTIONTYPE_MULTICAST )
            continue; // Don't use multicast for primary connections

        ConnectionPtr connection = Connection::create( description );
        if( !connection || !connection->connect( ))
            continue;

        return _connect( node, connection );
    }

    LBWARN << "Node unreachable, all connections failed to connect" <<std::endl;
    return CONNECT_UNREACHABLE;
}

bool LocalNode::connect( NodePtr node, ConnectionPtr connection )
{
    return ( _connect( node, connection ) == CONNECT_OK );
}

uint32_t LocalNode::_connect( NodePtr node, ConnectionPtr connection )
{
    LBASSERT( connection );
    LBASSERT( node->getNodeID() != getNodeID( ));

    if( !node || !isListening() || !connection->isConnected() ||
        !node->isClosed( ))
    {
        return CONNECT_BAD_STATE;
    }

    _addConnection( connection );

    // send connect command to peer
    lunchbox::Request< bool > request = registerRequest< bool >( node.get( ));
#ifdef COLLAGE_BIGENDIAN
    uint32_t cmd = CMD_NODE_CONNECT_BE;
    lunchbox::byteswap( cmd );
#else
    const uint32_t cmd = CMD_NODE_CONNECT;
#endif
    OCommand( Connections( 1, connection ), cmd )
        << getNodeID() << request << getType() << serialize();

    bool connected = false;
    try
    {
        connected = request.wait( 10000 /*ms*/ );
    }
    catch( lunchbox::FutureTimeout& )
    {
        LBWARN << "Node connection handshake timeout - " << node
               << " not a Collage node?" << std::endl;
        request.relinquish();
        return CONNECT_TIMEOUT;
    }

    // In simultaneous connect case, depending on the connection type
    // (e.g. RDMA), a check on the connection state of the node is required
    if( !connected || !node->isConnected( ))
        return CONNECT_TRY_AGAIN;

    LBASSERT( node->getNodeID() != 0 );
    LBASSERTINFO( node->getNodeID() != getNodeID(), getNodeID() );
    LBINFO << node << " connected to " << *(Node*)this << std::endl;
    return CONNECT_OK;
}

NodePtr LocalNode::connectObjectMaster( const uint128_t& id )
{
    LBASSERTINFO( id.isUUID(), id );
    if( !id.isUUID( ))
    {
        LBWARN << "Invalid object id " << id << std::endl;
        return 0;
    }

    const NodeID masterNodeID = _impl->objectStore->findMasterNodeID( id );
    if( masterNodeID == 0 )
    {
        LBWARN << "Can't find master node for object " << id << std::endl;
        return 0;
    }

    NodePtr master = connect( masterNodeID );
    if( master && !master->isClosed( ))
        return master;

    LBWARN << "Can't connect master node with id " << masterNodeID
           << " for object " << id << std::endl;
    return 0;
}

NodePtr LocalNode::createNode( const uint32_t type )
{
    LBASSERTINFO( type == NODETYPE_NODE, type );
    return new Node( type );
}

NodePtr LocalNode::getNode( const NodeID& id ) const
{
    lunchbox::ScopedFastRead mutex( _impl->nodes );
    NodeHash::const_iterator i = _impl->nodes->find( id );
    if( i == _impl->nodes->end() || !i->second->isReachable( ))
        return 0;
    return i->second;
}

void LocalNode::getNodes( Nodes& nodes, const bool addSelf ) const
{
    lunchbox::ScopedFastRead mutex( _impl->nodes );
    for( NodeHashCIter i = _impl->nodes->begin(); i != _impl->nodes->end(); ++i)
    {
        NodePtr node = i->second;
        if( node->isReachable() && ( addSelf || node != this ))
            nodes.push_back( i->second );
    }
}

CommandQueue* LocalNode::getCommandThreadQueue()
{
    return _impl->commandThread->getWorkerQueue();
}

bool LocalNode::inCommandThread() const
{
    return _impl->commandThread->isCurrent();
}

int64_t LocalNode::getTime64() const
{
    return _impl->clock.getTime64();
}

ssize_t LocalNode::getCounter( const Counter counter ) const
{
    return _impl->counters[ counter ];
}

void LocalNode::flushCommands()
{
    _impl->incoming.interrupt();
}

//----------------------------------------------------------------------
// receiver thread functions
//----------------------------------------------------------------------
void LocalNode::_runReceiverThread()
{
    LB_TS_THREAD( _rcvThread );
    _initService();

    int nErrors = 0;
    while( isListening( ))
    {
        const ConnectionSet::Event result = _impl->incoming.select();
        switch( result )
        {
            case ConnectionSet::EVENT_CONNECT:
                _handleConnect();
                break;

            case ConnectionSet::EVENT_DATA:
                _handleData();
                break;

            case ConnectionSet::EVENT_DISCONNECT:
            case ConnectionSet::EVENT_INVALID_HANDLE:
                _handleDisconnect();
                break;

            case ConnectionSet::EVENT_TIMEOUT:
                LBINFO << "select timeout" << std::endl;
                break;

            case ConnectionSet::EVENT_ERROR:
                ++nErrors;
                LBWARN << "Connection error during select" << std::endl;
                if( nErrors > 100 )
                {
                    LBWARN << "Too many errors in a row, capping connection"
                           << std::endl;
                    _handleDisconnect();
                }
                break;

            case ConnectionSet::EVENT_SELECT_ERROR:
                LBWARN << "Error during select" << std::endl;
                ++nErrors;
                if( nErrors > 10 )
                {
                    LBWARN << "Too many errors in a row" << std::endl;
                    LBUNIMPLEMENTED;
                }
                break;

            case ConnectionSet::EVENT_INTERRUPT:
                _redispatchCommands();
                break;

            default:
                LBUNIMPLEMENTED;
        }
        if( result != ConnectionSet::EVENT_ERROR &&
            result != ConnectionSet::EVENT_SELECT_ERROR )
        {
            nErrors = 0;
        }
    }

    if( !_impl->pendingCommands.empty( ))
        LBWARN << _impl->pendingCommands.size()
               << " commands pending while leaving command thread" << std::endl;

    _impl->pendingCommands.clear();
    LBCHECK( _impl->commandThread->join( ));

    ConnectionPtr connection = getConnection();
    PipeConnectionPtr pipe = LBSAFECAST( PipeConnection*, connection.get( ));
    connection = pipe->getSibling();
    _removeConnection( connection );
    _impl->connectionNodes.erase( connection );
    _disconnect();

    const Connections& connections = _impl->incoming.getConnections();
    while( !connections.empty( ))
    {
        connection = connections.back();
        NodePtr node = _impl->connectionNodes[ connection ];

        if( node )
            _closeNode( node );
        _removeConnection( connection );
    }

    _impl->objectStore->clear();
    _impl->pendingCommands.clear();
    _impl->smallBuffers.flush();
    _impl->bigBuffers.flush();

    LBINFO << "Leaving receiver thread of " << lunchbox::className( this )
           << std::endl;
}

void LocalNode::_handleConnect()
{
    ConnectionPtr connection = _impl->incoming.getConnection();
    ConnectionPtr newConn = connection->acceptSync();
    connection->acceptNB();

    if( newConn )
        _addConnection( newConn );
    else
        LBINFO << "Received connect event, but accept() failed" << std::endl;
}

void LocalNode::_handleDisconnect()
{
    while( _handleData( )) ; // read remaining data off connection

    ConnectionPtr connection = _impl->incoming.getConnection();
    ConnectionNodeHash::iterator i = _impl->connectionNodes.find( connection );

    if( i != _impl->connectionNodes.end( ))
    {
        NodePtr node = i->second;

        node->ref(); // extend lifetime to give cmd handler a chance

        // local command dispatching
        OCommand( this, this, CMD_NODE_REMOVE_NODE )
                << node.get() << uint32_t( LB_UNDEFINED_UINT32 );

        if( node->getConnection() == connection )
            _closeNode( node );
        else if( connection->isMulticast( ))
            node->_removeMulticast( connection );
    }

    _removeConnection( connection );
}

bool LocalNode::_handleData()
{
    _impl->smallBuffers.compact();
    _impl->bigBuffers.compact();

    ConnectionPtr connection = _impl->incoming.getConnection();
    LBASSERT( connection );

    BufferPtr buffer = _readHead( connection );
    if( !buffer ) // fluke signal
        return false;

    ICommand command = _setupCommand( connection, buffer );
    const bool gotCommand = _readTail( command, buffer, connection );
    LBASSERT( gotCommand );

    // start next receive
    BufferPtr nextBuffer = _impl->smallBuffers.alloc( COMMAND_ALLOCSIZE );
    connection->recvNB( nextBuffer, COMMAND_MINSIZE );

    if( gotCommand )
    {
        _dispatchCommand( command );
        return true;
    }

    LBERROR << "Incomplete command read: " << command << std::endl;
    return false;
}

BufferPtr LocalNode::_readHead( ConnectionPtr connection )
{
    BufferPtr buffer;
    const bool gotSize = connection->recvSync( buffer, false );

    if( !buffer ) // fluke signal
    {
        LBWARN << "Erronous network event on " << connection->getDescription()
               << std::endl;
        _impl->incoming.setDirty();
        return 0;
    }

    if( gotSize )
        return buffer;

    // Some systems signal data on dead connections.
    buffer->setSize( 0 );
    connection->recvNB( buffer, COMMAND_MINSIZE );
    return 0;
}

ICommand LocalNode::_setupCommand( ConnectionPtr connection,
                                   ConstBufferPtr buffer )
{
    NodePtr node;
    ConnectionNodeHashCIter i = _impl->connectionNodes.find( connection );
    if( i != _impl->connectionNodes.end( ))
        node = i->second;
    LBVERB << "Handle data from " << node << std::endl;

#ifdef COLLAGE_BIGENDIAN
    const bool swapping = node ? !node->isBigEndian() : false;
#else
    const bool swapping = node ? node->isBigEndian() : false;
#endif
    ICommand command( this, node, buffer, swapping );

    if( node )
    {
        node->_setLastReceive( getTime64( ));
        return command;
    }

    uint32_t cmd = command.getCommand();
#ifdef COLLAGE_BIGENDIAN
    lunchbox::byteswap( cmd ); // pre-node commands are sent little endian
#endif
    switch( cmd )
    {
    case CMD_NODE_CONNECT:
    case CMD_NODE_CONNECT_REPLY:
    case CMD_NODE_ID:
#ifdef COLLAGE_BIGENDIAN
        command = ICommand( this, node, buffer, true );
#endif
        break;

    case CMD_NODE_CONNECT_BE:
    case CMD_NODE_CONNECT_REPLY_BE:
    case CMD_NODE_ID_BE:
#ifndef COLLAGE_BIGENDIAN
        command = ICommand( this, node, buffer, true );
#endif
        break;

    default:
        LBUNIMPLEMENTED;
        return ICommand();
    }

    command.setCommand( cmd ); // reset correctly swapped version
    return command;
}

bool LocalNode::_readTail( ICommand& command, BufferPtr buffer,
                           ConnectionPtr connection )
{
    const uint64_t needed = command.getSize();
    if( needed <= buffer->getSize( ))
        return true;

    if( needed > buffer->getMaxSize( ))
    {
        LBASSERT( needed > COMMAND_ALLOCSIZE );
        LBASSERTINFO( needed < LB_BIT48,
                      "Out-of-sync network stream: " << command << "?" );
        // not enough space for remaining data, alloc and copy to new buffer
        BufferPtr newBuffer = _impl->bigBuffers.alloc( needed );
        newBuffer->replace( *buffer );
        buffer = newBuffer;

        command = ICommand( this, command.getRemoteNode(), buffer,
                            command.isSwapping( ));
    }

    // read remaining data
    connection->recvNB( buffer, command.getSize() - buffer->getSize( ));
    return connection->recvSync( buffer );
}

BufferPtr LocalNode::allocBuffer( const uint64_t size )
{
    LBASSERT( _impl->receiverThread->isStopped() || _impl->inReceiverThread( ));
    BufferPtr buffer = size > COMMAND_ALLOCSIZE ?
        _impl->bigBuffers.alloc( size ) :
        _impl->smallBuffers.alloc( COMMAND_ALLOCSIZE );
    return buffer;
}

void LocalNode::_dispatchCommand( ICommand& command )
{
    LBASSERTINFO( command.isValid(), command );

    if( dispatchCommand( command ))
        _redispatchCommands();
    else
    {
        _redispatchCommands();
        _impl->pendingCommands.push_back( command );
    }
}

bool LocalNode::dispatchCommand( ICommand& command )
{
    LBVERB << "dispatch " << command << " by " << getNodeID() << std::endl;
    LBASSERTINFO( command.isValid(), command );

    const uint32_t type = command.getType();
    switch( type )
    {
        case COMMANDTYPE_NODE:
            LBCHECK( Dispatcher::dispatchCommand( command ));
            return true;

        case COMMANDTYPE_OBJECT:
            return _impl->objectStore->dispatchObjectCommand( command );

        default:
            LBABORT( "Unknown command type " << type << " for " << command );
            return true;
    }
}

void LocalNode::_redispatchCommands()
{
    bool changes = true;
    while( changes && !_impl->pendingCommands.empty( ))
    {
        changes = false;

        for( CommandList::iterator i = _impl->pendingCommands.begin();
             i != _impl->pendingCommands.end(); ++i )
        {
            ICommand& command = *i;
            LBASSERT( command.isValid( ));

            if( dispatchCommand( command ))
            {
                _impl->pendingCommands.erase( i );
                changes = true;
                break;
            }
        }
    }

#ifndef NDEBUG
    if( !_impl->pendingCommands.empty( ))
        LBVERB << _impl->pendingCommands.size() << " undispatched commands"
               << std::endl;
    LBASSERT( _impl->pendingCommands.size() < 200 );
#endif
}

void LocalNode::_initService()
{
    LB_TS_SCOPED( _rcvThread );
    _impl->service->withdraw(); // go silent during k/v update

    const ConnectionDescriptions& descs = getConnectionDescriptions();
    if( descs.empty( ))
        return;

    std::ostringstream out;
    out << getType();
    _impl->service->set( "co_type", out.str( ));

    out.str("");
    out << descs.size();
    _impl->service->set( "co_numPorts", out.str( ));

    for( ConnectionDescriptionsCIter i = descs.begin(); i != descs.end(); ++i )
    {
        ConnectionDescriptionPtr desc = *i;
        out.str("");
        out << "co_port" << i - descs.begin();
        _impl->service->set( out.str(), desc->toString( ));
    }

    _impl->service->announce( descs.front()->port, getNodeID().getString( ));
}

void LocalNode::_exitService()
{
    _impl->service->withdraw();
}

Zeroconf LocalNode::getZeroconf()
{
    lunchbox::ScopedWrite mutex( _impl->service );
    _impl->service->discover( lunchbox::Servus::IF_ALL, 500 );
    return Zeroconf( _impl->service.data );
}


//----------------------------------------------------------------------
// command thread functions
//----------------------------------------------------------------------
bool LocalNode::_startCommandThread( const int32_t threadID )
{
    _impl->commandThread->threadID = threadID;
    return _impl->commandThread->start();
}

bool LocalNode::_notifyCommandThreadIdle()
{
    return _impl->objectStore->notifyCommandThreadIdle();
}

bool LocalNode::_cmdAckRequest( ICommand& command )
{
    const uint32_t requestID = command.get< uint32_t >();
    LBASSERT( requestID != LB_UNDEFINED_UINT32 );

    serveRequest( requestID );
    return true;
}

bool LocalNode::_cmdStopRcv( ICommand& command )
{
    LB_TS_THREAD( _rcvThread );
    LBASSERT( isListening( ));

    _exitService();
    _setClosing(); // causes rcv thread exit

    command.setCommand( CMD_NODE_STOP_CMD ); // causes cmd thread exit
    _dispatchCommand( command );
    return true;
}

bool LocalNode::_cmdStopCmd( ICommand& )
{
    LB_TS_THREAD( _cmdThread );
    LBASSERTINFO( isClosing(), *this );

    _setClosed();
    return true;
}

bool LocalNode::_cmdSetAffinity( ICommand& command )
{
    const int32_t affinity = command.get< int32_t >();

    lunchbox::Thread::setAffinity( affinity );
    return true;
}

bool LocalNode::_cmdConnect( ICommand& command )
{
    LBASSERTINFO( !command.getRemoteNode(), command );
    LBASSERT( _impl->inReceiverThread( ));

    const NodeID& nodeID = command.get< NodeID >();
    const uint32_t requestID = command.get< uint32_t >();
    const uint32_t nodeType = command.get< uint32_t >();
    std::string data = command.get< std::string >();

    LBVERB << "handle connect " << command << " req " << requestID << " type "
           << nodeType << " data " << data << std::endl;

    ConnectionPtr connection = _impl->incoming.getConnection();

    LBASSERT( connection );
    LBASSERT( nodeID != getNodeID() );
    LBASSERT( _impl->connectionNodes.find( connection ) ==
              _impl->connectionNodes.end( ));

    NodePtr peer;
#ifdef COLLAGE_BIGENDIAN
    uint32_t cmd = CMD_NODE_CONNECT_REPLY_BE;
    lunchbox::byteswap( cmd );
#else
    const uint32_t cmd = CMD_NODE_CONNECT_REPLY;
#endif

    // No locking needed, only recv thread modifies
    NodeHashCIter i = _impl->nodes->find( nodeID );
    if( i != _impl->nodes->end( ))
    {
        peer = i->second;
        if( peer->isReachable( ))
        {
            // Node exists, probably simultaneous connect from peer
            LBINFO << "Already got node " << nodeID << ", refusing connect"
                   << std::endl;

            // refuse connection
            OCommand( Connections( 1, connection ), cmd )
                << NodeID() << requestID;

            // NOTE: There is no close() here. The reply command above has to be
            // received by the peer first, before closing the connection.
            _removeConnection( connection );
            return true;
        }
    }

    // create and add connected node
    if( !peer )
        peer = createNode( nodeType );
    if( !peer )
    {
        LBINFO << "Can't create node of type " << nodeType << ", disconnecting"
               << std::endl;

        // refuse connection
        OCommand( Connections( 1, connection ), cmd ) << NodeID() << requestID;

        // NOTE: There is no close() here. The reply command above has to be
        // received by the peer first, before closing the connection.
        _removeConnection( connection );
        return true;
    }

    if( !peer->deserialize( data ))
        LBWARN << "Error during node initialization" << std::endl;
    LBASSERTINFO( data.empty(), data );
    LBASSERTINFO( peer->getNodeID() == nodeID,
                  peer->getNodeID() << "!=" << nodeID );
    LBASSERT( peer->getType() == nodeType );

    _impl->connectionNodes[ connection ] = peer;
    {
        lunchbox::ScopedFastWrite mutex( _impl->nodes );
        _impl->nodes.data[ peer->getNodeID() ] = peer;
    }
    LBVERB << "Added node " << nodeID << std::endl;

    // send our information as reply
    OCommand( Connections( 1, connection ), cmd )
        << getNodeID() << requestID << getType() << serialize();

    return true;
}

bool LocalNode::_cmdConnectReply( ICommand& command )
{
    LBASSERT( !command.getRemoteNode( ));
    LBASSERT( _impl->inReceiverThread( ));

    ConnectionPtr connection = _impl->incoming.getConnection();
    LBASSERT( _impl->connectionNodes.find( connection ) ==
              _impl->connectionNodes.end( ));

    const NodeID& nodeID = command.get< NodeID >();
    const uint32_t requestID = command.get< uint32_t >();

    // connection refused
    if( nodeID == 0 )
    {
        LBINFO << "Connection refused, node already connected by peer"
               << std::endl;

        _removeConnection( connection );
        serveRequest( requestID, false );
        return true;
    }

    const uint32_t nodeType = command.get< uint32_t >();
    std::string data = command.get< std::string >();

    LBVERB << "handle connect reply " << command << " req " << requestID
           << " type " << nodeType << " data " << data << std::endl;

    // No locking needed, only recv thread modifies
    NodeHash::const_iterator i = _impl->nodes->find( nodeID );
    NodePtr peer;
    if( i != _impl->nodes->end( ))
        peer = i->second;

    if( peer && peer->isReachable( )) // simultaneous connect
    {
        LBINFO << "Closing simultaneous connection from " << peer << " on "
               << connection << std::endl;

        _removeConnection( connection );
        _closeNode( peer );
        serveRequest( requestID, false );
        return true;
    }

    // create and add node
    if( !peer )
    {
        if( requestID != LB_UNDEFINED_UINT32 )
            peer = reinterpret_cast< Node* >( getRequestData( requestID ));
        else
            peer = createNode( nodeType );
    }
    if( !peer )
    {
        LBINFO << "Can't create node of type " << nodeType << ", disconnecting"
               << std::endl;
        _removeConnection( connection );
        return true;
    }

    LBASSERTINFO( peer->getType() == nodeType,
                  peer->getType() << " != " << nodeType );
    LBASSERT( peer->isClosed( ));

    if( !peer->deserialize( data ))
        LBWARN << "Error during node initialization" << std::endl;
    LBASSERT( data.empty( ));
    LBASSERT( peer->getNodeID() == nodeID );

    // send ACK to peer
    // cppcheck-suppress unusedScopedObject
    OCommand( Connections( 1, connection ), CMD_NODE_CONNECT_ACK );

    peer->_connect( connection );
    _impl->connectionNodes[ connection ] = peer;
    {
        lunchbox::ScopedFastWrite mutex( _impl->nodes );
        _impl->nodes.data[ peer->getNodeID() ] = peer;
    }
    _connectMulticast( peer );
    LBVERB << "Added node " << nodeID << std::endl;

    serveRequest( requestID, true );

    notifyConnect( peer );
    return true;
}

bool LocalNode::_cmdConnectAck( ICommand& command )
{
    NodePtr node = command.getRemoteNode();
    LBASSERT( node );
    LBASSERT( _impl->inReceiverThread( ));
    LBVERB << "handle connect ack" << std::endl;

    node->_connect( _impl->incoming.getConnection( ));
    _connectMulticast( node );
    notifyConnect( node );
    return true;
}

bool LocalNode::_cmdID( ICommand& command )
{
    LBASSERT( _impl->inReceiverThread( ));

    const NodeID& nodeID = command.get< NodeID >();
    uint32_t nodeType = command.get< uint32_t >();
    std::string data = command.get< std::string >();

    if( command.getRemoteNode( ))
    {
        LBASSERT( nodeID == command.getRemoteNode()->getNodeID( ));
        LBASSERT( command.getRemoteNode()->_getMulticast( ));
        return true;
    }

    LBINFO << "handle ID " << command << " node " << nodeID << std::endl;

    ConnectionPtr connection = _impl->incoming.getConnection();
    LBASSERT( connection->isMulticast( ));
    LBASSERT( _impl->connectionNodes.find( connection ) ==
              _impl->connectionNodes.end( ));

    NodePtr node;
    if( nodeID == getNodeID() ) // 'self' multicast connection
        node = this;
    else
    {
        // No locking needed, only recv thread writes
        NodeHash::const_iterator i = _impl->nodes->find( nodeID );

        if( i == _impl->nodes->end( ))
        {
            // unknown node: create and add unconnected node
            node = createNode( nodeType );

            if( !node->deserialize( data ))
                LBWARN << "Error during node initialization" << std::endl;
            LBASSERTINFO( data.empty(), data );

            {
                lunchbox::ScopedFastWrite mutex( _impl->nodes );
                _impl->nodes.data[ nodeID ] = node;
            }
            LBVERB << "Added node " << nodeID << " with multicast "
                   << connection << std::endl;
        }
        else
            node = i->second;
    }
    LBASSERT( node );
    LBASSERTINFO( node->getNodeID() == nodeID,
                  node->getNodeID() << "!=" << nodeID );

    _connectMulticast( node, connection );
    _impl->connectionNodes[ connection ] = node;
    LBINFO << "Added multicast connection " << connection << " from " << nodeID
           << " to " << getNodeID() << std::endl;
    return true;
}

bool LocalNode::_cmdDisconnect( ICommand& command )
{
    LBASSERT( _impl->inReceiverThread( ));

    const uint32_t requestID = command.get< uint32_t >();

    NodePtr node = static_cast<Node*>( getRequestData( requestID ));
    LBASSERT( node );

    _closeNode( node );
    LBASSERT( node->isClosed( ));
    serveRequest( requestID );
    return true;
}

bool LocalNode::_cmdGetNodeData( ICommand& command )
{
    const NodeID& nodeID = command.get< NodeID >();
    const uint32_t requestID = command.get< uint32_t >();

    LBVERB << "cmd get node data: " << command << " req " << requestID
           << " nodeID " << nodeID << std::endl;

    NodePtr node = getNode( nodeID );
    NodePtr toNode = command.getRemoteNode();

    uint32_t nodeType = NODETYPE_INVALID;
    std::string nodeData;
    if( node )
    {
        nodeType = node->getType();
        nodeData = node->serialize();
        LBINFO << "Sent node data '" << nodeData << "' for " << nodeID << " to "
               << toNode << std::endl;
    }

    toNode->send( CMD_NODE_GET_NODE_DATA_REPLY )
        << nodeID << requestID << nodeType << nodeData;
    return true;
}

bool LocalNode::_cmdGetNodeDataReply( ICommand& command )
{
    LBASSERT( _impl->inReceiverThread( ));

    const NodeID& nodeID = command.get< NodeID >();
    const uint32_t requestID = command.get< uint32_t >();
    const uint32_t nodeType = command.get< uint32_t >();
    std::string nodeData = command.get< std::string >();

    LBVERB << "cmd get node data reply: " << command << " req " << requestID
           << " type " << nodeType << " data " << nodeData << std::endl;

    // No locking needed, only recv thread writes
    NodeHash::const_iterator i = _impl->nodes->find( nodeID );
    if( i != _impl->nodes->end( ))
    {
        // Requested node connected to us in the meantime
        NodePtr node = i->second;

        node->ref( this );
        serveRequest( requestID, node.get( ));
        return true;
    }

    if( nodeType == NODETYPE_INVALID )
    {
        serveRequest( requestID, (void*)0 );
        return true;
    }

    // new node: create and add unconnected node
    NodePtr node = createNode( nodeType );
    if( node )
    {
        LBASSERT( node );

        if( !node->deserialize( nodeData ))
            LBWARN << "Failed to initialize node data" << std::endl;
        LBASSERT( nodeData.empty( ));
        node->ref( this );
    }
    else
        LBINFO << "Can't create node of type " << nodeType << std::endl;

    serveRequest( requestID, node.get( ));
    return true;
}

bool LocalNode::_cmdAcquireSendToken( ICommand& command )
{
    LBASSERT( inCommandThread( ));
    if( !_impl->sendToken ) // enqueue command if no token available
    {
        const uint32_t timeout = Global::getTimeout();
        if( timeout == LB_TIMEOUT_INDEFINITE ||
            ( getTime64() - _impl->lastSendToken <= timeout ))
        {
            _impl->sendTokenQueue.push_back( command );
            return true;
        }

        // timeout! - clear old requests
        _impl->sendTokenQueue.clear();
        // 'generate' new token - release is robust
    }

    _impl->sendToken = false;

    const uint32_t requestID = command.get< uint32_t >();
    command.getRemoteNode()->send( CMD_NODE_ACQUIRE_SEND_TOKEN_REPLY )
            << requestID;
    return true;
}

bool LocalNode::_cmdAcquireSendTokenReply( ICommand& command )
{
    const uint32_t requestID = command.get< uint32_t >();
    serveRequest( requestID );
    return true;
}

bool LocalNode::_cmdReleaseSendToken( ICommand& )
{
    LBASSERT( inCommandThread( ));
    _impl->lastSendToken = getTime64();

    if( _impl->sendToken )
        return true; // double release due to timeout
    if( _impl->sendTokenQueue.empty( ))
    {
        _impl->sendToken = true;
        return true;
    }

    ICommand& request = _impl->sendTokenQueue.front();

    const uint32_t requestID = request.get< uint32_t >();
    request.getRemoteNode()->send( CMD_NODE_ACQUIRE_SEND_TOKEN_REPLY )
            << requestID;
    _impl->sendTokenQueue.pop_front();
    return true;
}

bool LocalNode::_cmdAddListener( ICommand& command )
{
    Connection* rawConnection =
        reinterpret_cast< Connection* >( command.get< uint64_t >( ));
    std::string data = command.get< std::string >();

    ConnectionDescriptionPtr description = new ConnectionDescription( data );
    command.getRemoteNode()->_addConnectionDescription( description );

    if( command.getRemoteNode() != this )
        return true;

    ConnectionPtr connection = rawConnection;
    connection->unref();
    LBASSERT( connection );

    _impl->connectionNodes[ connection ] = this;
    if( connection->isMulticast( ))
        _addMulticast( this, connection );

    connection->acceptNB();
    _impl->incoming.addConnection( connection );

    _initService(); // update zeroconf
    return true;
}

bool LocalNode::_cmdRemoveListener( ICommand& command )
{
    const uint32_t requestID = command.get< uint32_t >();
    Connection* rawConnection = command.get< Connection* >();
    std::string data = command.get< std::string >();

    ConnectionDescriptionPtr description = new ConnectionDescription( data );
    LBCHECK(
          command.getRemoteNode()->_removeConnectionDescription( description ));

    if( command.getRemoteNode() != this )
        return true;

    _initService(); // update zeroconf

    ConnectionPtr connection = rawConnection;
    connection->unref( this );
    LBASSERT( connection );

    if( connection->isMulticast( ))
        _removeMulticast( connection );

    _impl->incoming.removeConnection( connection );
    LBASSERT( _impl->connectionNodes.find( connection ) !=
              _impl->connectionNodes.end( ));
    _impl->connectionNodes.erase( connection );
    serveRequest( requestID );
    return true;
}

bool LocalNode::_cmdPing( ICommand& command )
{
    LBASSERT( inCommandThread( ));
    command.getRemoteNode()->send( CMD_NODE_PING_REPLY );
    return true;
}

bool LocalNode::_cmdCommand( ICommand& command )
{
    const uint128_t& commandID = command.get< uint128_t >();
    CommandHandler func;
    {
        lunchbox::ScopedFastRead mutex( _impl->commandHandlers );
        CommandHashCIter i = _impl->commandHandlers->find( commandID );
        if( i == _impl->commandHandlers->end( ))
            return false;

        CommandQueue* queue = i->second.second;
        if( queue )
        {
            command.setDispatchFunction( CmdFunc( this,
                                                &LocalNode::_cmdCommandAsync ));
            queue->push( command );
            return true;
        }
        // else

        func = i->second.first;
    }

    CustomICommand customCmd( command );
    return func( customCmd );
}

bool LocalNode::_cmdCommandAsync( ICommand& command )
{
    const uint128_t& commandID = command.get< uint128_t >();
    CommandHandler func;
    {
        lunchbox::ScopedFastRead mutex( _impl->commandHandlers );
        CommandHashCIter i = _impl->commandHandlers->find( commandID );
        LBASSERT( i != _impl->commandHandlers->end( ));
        if( i ==  _impl->commandHandlers->end( ))
            return true; // deregistered between dispatch and now
        func = i->second.first;
    }
    CustomICommand customCmd( command );
    return func( customCmd );
}

bool LocalNode::_cmdAddConnection( ICommand& command )
{
    LBASSERT( _impl->inReceiverThread( ));

    ConnectionPtr connection = command.get< ConnectionPtr >();
    _addConnection( connection );
    connection->unref(); // ref'd by _addConnection
    return true;
}

}
