
/* Copyright (c) 2013-2014, Stefan.Eilemann@epfl.ch
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

// Tests network throughput for object operations
// Usage: see 'coObjectperf -h'

#include <co/co.h>
#include <co/connections.h>
#include <lunchbox/clock.h>
#include <lunchbox/lock.h>
#include <lunchbox/monitor.h>
#include <lunchbox/mtQueue.h>
#include <boost/foreach.hpp>
#pragma warning( disable: 4275 )
#include <boost/program_options.hpp>
#pragma warning( default: 4275 )
#include <iostream>

#ifndef MIN
#  define MIN LB_MIN
#endif

namespace po = boost::program_options;

static uint64_t objectSize = 0;
static uint64_t nObjects = 0;
static const size_t treeWidth = 10;

typedef lunchbox::Buffer< uint8_t > Buffer;

static const uint64_t _objectID( 0xa9188163b433428dull );
co::NodePtr master;

class Object;
typedef std::vector< Object* > Objects;

enum Command
{
    CMD_NODE_PARAMS = co::CMD_NODE_CUSTOM,
    CMD_NODE_SETUP,
    CMD_NODE_OPERATION
};

class Object : public co::Serializable
{
public:
    Object( const size_t nChildren, size_t index )
    {
        LBASSERT( nChildren < nObjects );
        LBASSERT( index < nObjects );
        setID( co::uint128_t( _objectID, index ));

        buffer_.resize( objectSize );
        const uint8_t* from = reinterpret_cast< uint8_t* >( this );
        for( size_t i = 0; i < objectSize; i += sizeof( this ))
            ::memcpy( &buffer_[i], from, LB_MIN( objectSize-i, sizeof(this) ));

        if( nChildren == 0 )
            return;

        size_t nGrandChildren = nChildren > treeWidth ? nChildren-treeWidth : 0;
        const size_t step = std::ceil( nGrandChildren / float( treeWidth ));
        const size_t width = std::min( treeWidth, nChildren );
        for( size_t i = 0; i < width; ++i )
        {
            const size_t size = std::min( step, nGrandChildren );
            nGrandChildren -= size;

            ++index;
            children_.push_back( new Object( size, index ));
            index += size;
        }
    }
    virtual ~Object() {}

    void register_( co::LocalNodePtr node )
    {
        node->registerObject( this );
        BOOST_FOREACH( Object* child, children_ )
            child->register_( node );
    }

    void map( co::LocalNodePtr local, co::NodePtr remote )
    {
        co::f_bool_t mapped = local->mapObject( this, getID(), remote );
        BOOST_FOREACH( Object* child, children_ )
            child->map( local, remote );
        LBASSERT( mapped );
    }

    void unmap( co::LocalNodePtr local )
    {
        local->unmapObject( this );
        BOOST_FOREACH( Object* child, children_ )
            child->unmap( local );
    }

protected:
    ChangeType getChangeType() const final { return INSTANCE; }

private:
    Buffer buffer_;
    Objects children_;

    //uint32_t chooseCompressor() const final { return EQ_COMPRESSOR_NONE; }

    void serialize( co::DataOStream& os, const uint64_t ) override
        { os << buffer_; }
    void deserialize( co::DataIStream& is, const uint64_t ) override
        { is >> buffer_; }
};

class OperationIF
{
public:
    OperationIF( const std::string& name ) : name_( name ) {}
    virtual ~OperationIF() {}
    virtual void activate() = 0;
    virtual void print( const float time ) = 0;
    virtual void process() = 0;

protected:
    const std::string name_;
};

typedef std::vector< OperationIF* > Operations;

// Mapping
class MapLocalOp : public OperationIF
{
public:
    MapLocalOp( co::LocalNodePtr node, const std::string& name )
        : OperationIF( name )
        , node_( node )
        , last_( 0 )
    {}
    virtual ~MapLocalOp() {}

    void activate() final
    {
        last_ = node_->getCounter( co::LocalNode::COUNTER_MAP_OBJECT_REMOTE );
    }

    void print( const float time ) final
    {
        const ssize_t counter =
            node_->getCounter( co::LocalNode::COUNTER_MAP_OBJECT_REMOTE );
        const ssize_t nOps = counter - last_;
        last_ = counter;
        if( nOps <= 0 )
            return;

        const float mbps = objectSize/1024.0f/1024.0f * nOps / time*1000.f;
        std::cout << name_ << " " << nOps / time << " maps/ms, " << mbps
                  << " MB/s" << std::endl;
    }

    void process() final { /* NOP */ }

private:
    co::LocalNodePtr node_;
    ssize_t last_;
};

class MapRemoteOp : public OperationIF
{
public:
    MapRemoteOp( co::LocalNodePtr local, co::NodePtr remote,
                 const std::string& name = "slave  map all" )
        : OperationIF( name )
        , local_( local )
        , remote_( remote )
        , root_( nObjects-1, 0 )
    {}

    virtual ~MapRemoteOp() {}

    void activate() final { /* NOP */ }

    void print( const float time ) final
    {
        if( nOps_ == 0 )
            return;

        const float mbps = objectSize/1024.0f/1024.0f * nObjects*nOps_ /
                           time*1000.f;
        std::cout << name_ << " " << nOps_ * nObjects / time << " maps/ms, "
                  << mbps << " MB/s" << std::endl;
        nOps_ = 0;
    }

    void process() override
    {
        root_.map( local_, remote_ );
        root_.unmap( local_ );
        ++nOps_;
    }

protected:
    co::LocalNodePtr local_;
    co::NodePtr remote_;
    Object root_;
    static size_t nOps_;
};

size_t MapRemoteOp::nOps_ = 0;

class MapRemoteSingleOp : public MapRemoteOp
{
public:
    MapRemoteSingleOp( co::LocalNodePtr local, co::NodePtr remote )
        : MapRemoteOp( local, remote, "slave  map one" )
    {}
    virtual ~MapRemoteSingleOp() {}

    void process() final
    {
        if( remote_ == master )
            MapRemoteOp::process();
    }
};

class Node : public co::Node
{
public:
    enum State
    {
        STATE_NONE,
        STATE_SETUP,
        STATE_RUNNING
    };

    Node() : co::Node( 0xB0A ), active_( STATE_NONE ) {}

    void setup( co::LocalNodePtr local )
    {
        impls_.push_back( new MapRemoteOp( local, this ));
        impls_.push_back( new MapRemoteSingleOp( local, this ));
        // impls_.push_back( new PushLocalOp( this ));
        // impls_.push_back( new SyncLocalOp( this ));
        impls_[ active_ ]->activate();
    }

    void process()
    {
        LBASSERT( !impls_.empty( ));
        impls_[ active_ ]->process();
    }

    void switchOperation( const size_t next )
    {
        if( next >= impls_.size( )) // may happen during join
            return;

        impls_[ active_ ]->print( clock_.resetTimef( ));
        active_ = next % impls_.size();
        impls_[ active_ ]->activate();
    }

    void setState( const State state ) { state_ = state; }
    void waitState( const State state ) const { state_.waitGE( state ); }

private:
    size_t active_;
    Operations impls_;
    lunchbox::Clock clock_;
    lunchbox::Monitorb state_;
};

typedef lunchbox::RefPtr< Node > NodePtr;
typedef std::vector< NodePtr > Nodes;
typedef Nodes::iterator NodesIter;
typedef lunchbox::MTQueue< NodePtr > NodeQueue;
NodeQueue nodeQ_;

class LocalNode : public co::LocalNode
{
public:
    LocalNode( int argc, char **argv )
        : co::LocalNode( 0xB0A )
        , root_( 0 )
        , active_( 0 )
        , canChange_( true )
    {
        disableInstanceCache();
        // Add default listener so others can connect to me
        addConnectionDescription( new co::ConnectionDescription );

        if( !initLocal( argc, argv ))
        {
            co::exit();
            ::exit( EXIT_FAILURE );
        }
        getZeroconf().set( "coObjectperf", co::Version::getString( ));
        registerCommand( CMD_NODE_PARAMS, co::CommandFunc< LocalNode >( this,
                                                   &LocalNode::cmdParams_), 0 );
        registerCommand( CMD_NODE_SETUP, co::CommandFunc< LocalNode >( this,
                                                    &LocalNode::cmdSetup_), 0 );
        registerCommand( CMD_NODE_OPERATION, co::CommandFunc< LocalNode >( this,
                                                &LocalNode::cmdOperation_), 0 );
    }

    void setup()
    {
        root_ = new Object( nObjects-1, 0 );
        root_->register_( this );
        impls_.push_back( new MapLocalOp( this, "master map all" ));
        impls_.push_back( new MapLocalOp( this, "master map one" ));
        // impls_.push_back( new PushLocalOp( this ));
        // impls_.push_back( new SyncLocalOp( this ));
        impls_[ active_ ]->activate();
    }

    void process()
    {
        LBASSERT( !impls_.empty( ));

        canChange_ = false;
        impls_[ active_ ]->process();
    }

    void switchOperation()
    {
        co::Nodes nodes;
        getNodes( nodes );
        const co::Connections& connections = co::gatherConnections( nodes );
        co::OCommand( connections, CMD_NODE_OPERATION )
            << uint64_t( (active_+1) % impls_.size() );
    }

private:
    Object* root_;
    size_t active_;
    Operations impls_;
    lunchbox::Clock clock_;
    bool canChange_;

    virtual ~LocalNode()
    {
        BOOST_FOREACH( OperationIF* operation, impls_ )
            delete operation;
        delete root_;
    }

    co::NodePtr createNode( const uint32_t type ) override
    {
        return type == 0xB0A ? new ::Node : 0;
    }

    void notifyConnect( co::NodePtr node ) override
    {
        if( node->getType() == 0xB0A )
            nodeQ_.push( static_cast< ::Node* >( node.get( )));
    }

    void notifyDisconnect( co::NodePtr node ) override
    {
        if( node->getType() == 0xB0A )
            nodeQ_.push( static_cast< ::Node* >( node.get( )));
    }

    bool cmdParams_( co::ICommand& command )
    {
        NodePtr node = static_cast< ::Node* >( command.getNode().get( ));
        const uint64_t size = command.get< uint64_t >();
        const uint64_t num = command.get< uint64_t >();
        const bool canChange = command.get< bool >();

        if( size != objectSize || num != nObjects )
        {
            if( canChange && canChange_ )
            {
                // If both can change, take set values from bigger node id
                if( objectSize == 0 )
                    objectSize = size;
                if( nObjects <= treeWidth )
                    nObjects = num;
                if( getNodeID() < command.getNode()->getNodeID( ))
                {
                    if( size != 0 )
                        objectSize = size;
                    if( num != 0 )
                        nObjects = num;
                }
            }
            else if( canChange_ )
            {
                if( size != 0 )
                    objectSize = size;
                if( num != 0 )
                    nObjects = num;
            }
            else
            {
                LBASSERTINFO( canChange,
                         "Unhandled parameter mismatch between running nodes" );
            }
        }
        if( objectSize == 0 ) // setup defaults
            objectSize = 10000;
        if( nObjects <= treeWidth ) // setup defaults
            nObjects = 10000;
        LBINFO << "Using " << nObjects << " objects of " << objectSize << "b"
               << std::endl;
        node->setState( ::Node::STATE_SETUP );
        return true;
    }

    bool cmdSetup_( co::ICommand& command )
    {
        NodePtr node = static_cast< ::Node* >( command.getNode().get( ));
        node->setState( ::Node::STATE_RUNNING );
        return true;
    }

    bool cmdOperation_( co::ICommand& command )
    {
        const uint64_t next = command.get< uint64_t >();
        if( next >= impls_.size( )) // may happen during join
            return true;

        impls_[ active_ ]->print( clock_.resetTimef( ));
        active_ = next % impls_.size();
        impls_[ active_ ]->activate();

        co::Nodes nodes;
        getNodes( nodes, false );
        BOOST_FOREACH( co::NodePtr coNode, nodes )
        {
            if( coNode->getType() != 0xB0A )
                continue;
            NodePtr node = static_cast< ::Node* >( coNode.get( ));
            node->switchOperation( next );
        }
        std::cout << std::endl;
        return true;
    }
};
typedef lunchbox::RefPtr< LocalNode > LocalNodePtr;

int main( int argc, char **argv )
{
    if( !co::init( argc, argv ))
        return EXIT_FAILURE;

    co::ConnectionDescriptionPtr remote;
    bool useZeroconf = true;

    try // command line parsing
    {
        po::options_description options(
            "objectperf - Collage object data distribution benchmark tool "
            + co::Version::getString( ));

        std::string remoteString("");
        bool showHelp(false);
        bool disableZeroconf(false);

        options.add_options()
            ( "help,h",       po::bool_switch(&showHelp)->default_value(false),
              "show help message" )
            ( "connect,c",    po::value<std::string>(&remoteString),
              "connect to remote node, implies --disableZeroconf. Format IP[:port][:protocol]" )
            ( "disableZeroconf,d", po::bool_switch(&disableZeroconf)->default_value(false),
              "Disable automatic connect using zeroconf" )
            ( "objectSize,o", po::value<uint64_t>(&objectSize),
              "object size" )
            ( "numObjects,n", po::value<uint64_t>(&nObjects),
              "number of objects" );

        // parse program options
        po::variables_map variableMap;
        po::store( po::command_line_parser( argc, argv ).options(
                       options ).allow_unregistered().run(), variableMap );
        po::notify( variableMap );

        // evaluate parsed arguments
        if( showHelp )
        {
            std::cout << options << std::endl;
            co::exit();
            return EXIT_SUCCESS;
        }

        if( variableMap.count( "connect" ) == 1)
        {
            remote = new co::ConnectionDescription;
            remote->port = 4242;
            remote->fromString( remoteString );
        }

        if( disableZeroconf )
            useZeroconf = false;
    }
    catch( std::exception& exception )
    {
        std::cerr << "Command line parse error: " << exception.what()
                  << std::endl;
        co::exit();
        return EXIT_FAILURE;
    }


    // Set up local node
    const uint64_t size = objectSize;
    const uint64_t num = nObjects;

    LocalNodePtr localNode = new LocalNode( argc, argv );

    // connect at least one node
    if( remote )
    {
        co::NodePtr node = new Node;
        node->addConnectionDescription( remote );
        localNode->connect( node );
    }

    Nodes nodes;
    while( nodes.empty( ))
    {
        if( useZeroconf )
        {
            co::Zeroconf zeroconf = localNode->getZeroconf();
            const co::Strings& instances =
                localNode->getZeroconf().getInstances();
            BOOST_FOREACH( const std::string& instance, instances )
                if( !zeroconf.get( instance, "coObjectperf" ).empty( ))
                    localNode->connect( co::NodeID( instance ));
        }

        NodePtr node;
        while( nodeQ_.tryPop( node ))
        {
            LBASSERT( node->isConnected( ));
            if( node->isConnected( ))
            {
                node->send( CMD_NODE_PARAMS ) << size << num << true;
                node->waitState( Node::STATE_SETUP );
                if( nodes.empty( ))
                    localNode->setup();
                node->send( CMD_NODE_SETUP );
                node->waitState( Node::STATE_RUNNING );
                node->setup( localNode );
                nodes.push_back( node );
            }
        }
        lunchbox::Thread::yield();
    }

    // run
    master = localNode;
    BOOST_FOREACH( NodePtr node, nodes )
        if( master->getNodeID() > node->getNodeID( ))
            master = node;
    LBINFO << nodes.size() << " peers" << std::endl;

    lunchbox::Clock clock;
    while( !nodes.empty( ))
    {
        localNode->process();
        BOOST_FOREACH( NodePtr node, nodes )
            node->process();

        if( master == localNode && clock.getTime64() > 5000 )
        {
            localNode->switchOperation();
            clock.reset();
        }

        if( !nodeQ_.isEmpty( ))
        {
            NodePtr node;
            while( nodeQ_.tryPop( node ))
            {
                if( node->isConnected( ))
                {
                    node->send( CMD_NODE_PARAMS ) << objectSize << nObjects
                                                  << false;
                    node->send( CMD_NODE_SETUP );
                    node->waitState( Node::STATE_RUNNING );
                    node->setup( localNode );
                    nodes.push_back( node );
                }
                else
                {
                    NodesIter i = lunchbox::find( nodes, node );
                    if( i != nodes.end( ))
                        nodes.erase( i );
                }
            }

            master = localNode;
            BOOST_FOREACH( node, nodes )
                if( master->getNodeID() > node->getNodeID( ))
                    master = node;
            LBINFO << nodes.size() << " peers" << std::endl;
        }
    }

    LBCHECK( localNode->exitLocal( ));
    LBCHECK( co::exit( ));
    return EXIT_SUCCESS;
}
