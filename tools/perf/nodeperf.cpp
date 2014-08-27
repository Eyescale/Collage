
/* Copyright (c) 2012-2014, Stefan.Eilemann@epfl.ch
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

// Tests network throughput between co::Nodes
// Usage: see 'coNodeperf -h'

#include <co/co.h>
#include <lunchbox/clock.h>
#include <lunchbox/spinLock.h>
#include <lunchbox/scopedMutex.h>
#include <lunchbox/sleep.h>
#include <boost/foreach.hpp>
#pragma warning( disable: 4275 )
#include <boost/program_options.hpp>
#pragma warning( default: 4275 )
#include <iostream>

#ifndef MIN
#  define MIN LB_MIN
#endif

namespace po = boost::program_options;

namespace
{
class PerfNode;
typedef lunchbox::Lockable< co::Nodes, lunchbox::SpinLock > ConnectedNodes;
typedef lunchbox::Buffer< uint64_t > Buffer;

ConnectedNodes nodes_;
lunchbox::Lock print_;
static co::uint128_t _objectID( 0x25625429A197D730ull, 0x79F60861189007D5ull );
template< class C >
bool commandHandler( C command, Buffer& buffer, const uint64_t seed );

class Object : public co::Serializable
{
private:
    Buffer buffer_;

    void attach( const co::uint128_t& id, const uint32_t instanceID ) override
    {
        co::Serializable::attach( id, instanceID );

        registerCommand( co::CMD_OBJECT_CUSTOM,
                         co::CommandFunc< Object >( this, &Object::_cmdCustom ),
                         getLocalNode()->getCommandThreadQueue( ));
    }

    bool _cmdCustom( co::ICommand& command )
    {
        return commandHandler< co::ObjectICommand >( command, buffer_,
                                                    getID().low( ));
    }
};

class PerfNodeProxy : public co::Node
{
public:
    PerfNodeProxy() : co::Node( 0xC0FFEEu ), nPackets( 0 ) {}

    uint32_t nPackets;
    Object object;
};
typedef lunchbox::RefPtr< PerfNodeProxy > PerfNodeProxyPtr;

class PerfNode : public co::LocalNode
{
public:
    PerfNode() : co::LocalNode( 0xC0FFEEu )
    {
        registerCommand( co::CMD_NODE_CUSTOM,
                         co::CommandFunc< PerfNode >( this,
                                                      &PerfNode::_cmdCustom ),
                         getCommandThreadQueue( ));
    }

private:
    Buffer buffer_;

    co::NodePtr createNode( const uint32_t type ) override
        { return type == 0xC0FFEEu ? new PerfNodeProxy : new co::Node( type ); }

    void notifyConnect( co::NodePtr node ) override
    {
        LBINFO << "Connect from " << node << std::endl;
        lunchbox::ScopedFastWrite _mutex( nodes_ );
        nodes_->push_back( node );
    }

    void notifyDisconnect( co::NodePtr node ) override
    {
        lunchbox::ScopedFastWrite _mutex( nodes_ );
        co::NodesIter i = std::find( nodes_->begin(), nodes_->end(), node );
        if( i != nodes_->end( ))
            nodes_->erase( i );
        LBINFO << "Disconnected from " << node << std::endl;
    }

    bool _cmdCustom( co::ICommand& command )
    {
        return commandHandler( command, buffer_, getNodeID().low( ));
    }
};

template< class C >
bool commandHandler( C command, Buffer& buffer, const uint64_t seed )
{
    co::NodePtr node = command.getNode();
    if( node->getType() != 0xC0FFEEu )
        return false;

    PerfNodeProxyPtr peer = static_cast< PerfNodeProxy* >( node.get( ));
    const uint32_t nPackets = command.template get< uint32_t >();

    if( peer->nPackets != 0 && peer->nPackets - 1 != nPackets )
    {
        LBERROR << "Got packet " << nPackets << ", expected "
                << peer->nPackets - 1 << std::endl;
        return false;
    }
    peer->nPackets = nPackets;

    command >> buffer;
    const size_t i = ( seed + nPackets ) % buffer.getSize();
    if( buffer[ i ] != nPackets )
    {
        LBERROR << "Got " << buffer[ i ] << " @ " << i << ", expected "
                << nPackets <<  " in buffer of size " << buffer.getSize()
                << std::endl;
        return false;
    }
    return true;
}
}

int main( int argc, char **argv )
{
    if( !co::init( argc, argv ))
        return EXIT_FAILURE;

    co::ConnectionDescriptionPtr remote;
    size_t packetSize = 1048576u; // needs to be modulo 8
    uint32_t nPackets = 0xFFFFFFFFu;
    uint32_t waitTime = 0;
    bool useZeroconf = true;
    bool useObjects = false;

    try // command line parsing
    {
        po::options_description options(
            "nodeperf - Collage node-to-node network benchmark tool "
            + co::Version::getString( ));

        std::string remoteString("");
        bool showHelp(false);
        bool disableZeroconf(false);

        options.add_options()
            ( "help,h",       po::bool_switch(&showHelp)->default_value(false),
              "show help message" )
            ( "connect,c",    po::value<std::string>(&remoteString),
              "connect to remote node, implies --disableZeroconf. Format IP[:port][:protocol]" )
            ( "disableZeroconf,d",
              po::bool_switch(&disableZeroconf)->default_value(false),
              "Disable automatic connect using zeroconf" )
            ( "object,o",    po::bool_switch(&useObjects)->default_value(false),
              "Benchmark object-object instead of node-node communication" )
            ( "packetSize,p",  po::value<std::size_t>(&packetSize),
              "packet size" )
            ( "numPackets,n", po::value<uint32_t>(&nPackets),
              "number of packets to send" )
            ( "wait,w",       po::value<uint32_t>(&waitTime),
              "wait time (ms) between sends" );

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

        if( variableMap.count("connect") == 1 )
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
    co::LocalNodePtr localNode = new PerfNode;
    if( !localNode->initLocal( argc, argv ))
    {
        co::exit();
        return EXIT_FAILURE;
    }
    localNode->getZeroconf().set( "coNodeperf", co::Version::getString( ));

    Object object;
    object.setID( co::uint128_t( _objectID + localNode->getNodeID( )));
    LBCHECK( localNode->registerObject( &object ));

    // run
    if( remote )
    {
        co::NodePtr node = new PerfNodeProxy;
        node->addConnectionDescription( remote );
        localNode->connect( node );
    }
    else if( useZeroconf )
    {
        co::Zeroconf zeroconf = localNode->getZeroconf();
        const co::Strings& instances = localNode->getZeroconf().getInstances();
        BOOST_FOREACH( const std::string& instance, instances )
        {
            if( !zeroconf.get( instance, "coNodeperf" ).empty( ))
                localNode->connect( co::NodeID( instance ));
        }
    }
    {
        lunchbox::ScopedFastRead _mutex( nodes_ );
        if( nodes_->empty( ))
            // Add default listener so others can connect to me
            localNode->addListener( new co::ConnectionDescription );
    }

    co::Nodes nodes;
    while( true )
    {
        lunchbox::Thread::yield();

        lunchbox::ScopedFastRead _mutex( nodes_ );
        if( !nodes_->empty( ))
            break;
    }

    Buffer buffer;
    const size_t bufferElems = packetSize / sizeof( uint64_t );
    buffer.resize( bufferElems );
    for( size_t i = 0; i < bufferElems; ++i )
        buffer[i] = i;

    const float mBytesSec = buffer.getNumBytes() / 1024.0f / 1024.0f * 1000.0f;
    lunchbox::Clock clock;
    size_t sentPackets = 0;

    clock.reset();
    while( nPackets-- )
    {
        {
            lunchbox::ScopedFastRead _mutex( nodes_ );
            if( nodes != *nodes_ )
            {
                for( co::NodesCIter i = nodes_->begin(); i != nodes_->end();++i)
                {
                    co::NodePtr node = *i;
                    co::NodesCIter j = lunchbox::find( nodes, node );
                    if( j == nodes.end( ))
                    {
                        // new node, map perf object
                        LBASSERT( node->getType() == 0xC0FFEEu );
                        PerfNodeProxyPtr peer =
                                    static_cast< PerfNodeProxy* >( node.get( ));
                        LBCHECK( localNode->mapObject( &peer->object,
                                    co::uint128_t( _objectID +
                                                   peer->getNodeID( ))));
                    }
                }
                nodes = *nodes_;
            }
        }
        if( nodes.empty( ))
            break;

        for( co::NodesCIter i = nodes.begin(); i != nodes.end(); ++i )
        {
            co::NodePtr node = *i;
            if( node->getType() != 0xC0FFEEu )
                continue;

            if( useObjects )
            {
                const size_t j = (object.getID().low() + nPackets) %
                                 bufferElems;
                buffer[ j ] = nPackets;
                object.send( node, co::CMD_OBJECT_CUSTOM ) << nPackets <<buffer;
                buffer[ j ] = 0xDEADBEEFu;
            }
            else
            {
                const size_t j = (node->getNodeID().low() + nPackets) %
                                 bufferElems;
                buffer[ j ] = nPackets;
                node->send( co::CMD_NODE_CUSTOM ) << nPackets << buffer;
                buffer[ j ] = 0xDEADBEEFu;
            }
            ++sentPackets;

            if( waitTime > 0 )
                lunchbox::sleep( waitTime );
        }

        const float time = clock.getTimef();
        if( time > 1000.f )
        {
            const lunchbox::ScopedMutex<> mutex( print_ );
            std::cerr << "Send perf: " << mBytesSec / time * sentPackets
                      << "MB/s (" << sentPackets / time * 1000.f  << "pps)"
                      << std::endl;
            sentPackets = 0;
            clock.reset();
        }
    }

    const float time = clock.getTimef();
    if( time > 1000.f )
    {
        const lunchbox::ScopedMutex<> mutex( print_ );
        std::cerr << "Send perf: " << mBytesSec / time * sentPackets
                  << "MB/s (" << sentPackets / time * 1000.f  << "pps)"
                  << std::endl;
        sentPackets = 0;
        clock.reset();
    }

    localNode->deregisterObject( &object );
    LBCHECK( localNode->exitLocal( ));
    LBCHECK( co::exit( ));
    return EXIT_SUCCESS;
}
