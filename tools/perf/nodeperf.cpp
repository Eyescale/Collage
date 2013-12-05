
/* Copyright (c) 2012-2013, Stefan.Eilemann@epfl.ch
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
// Usage: see 'nodePerf -h'

#include <co/co.h>
#include <tclap/CmdLine.h>
#include <boost/foreach.hpp>
#include <iostream>

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

    virtual void attach( const co::UUID& id, const uint32_t instanceID )
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

    virtual co::NodePtr createNode( const uint32_t type )
        { return type == 0xC0FFEEu ? new PerfNodeProxy : new co::Node( type ); }

    virtual void notifyConnect( co::NodePtr node )
    {
        LBINFO << "Connect from " << node << std::endl;
        lunchbox::ScopedFastWrite _mutex( nodes_ );
        nodes_->push_back( node );
    }

    virtual void notifyDisconnect( co::NodePtr node )
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
        TCLAP::CmdLine command(
            "nodeperf - Collage node-to-node network benchmark tool", ' ',
            co::Version::getString( ));
        TCLAP::ValueArg< std::string > remoteArg( "c", "connect",
                            "connect to remote node, implies --disableZeroconf",
                                                  false, "",
                                                  "IP[:port][:protocol]",
                                                  command );
        TCLAP::SwitchArg zcArg( "d", "disableZeroconf",
                                "Disable automatic connect using zeroconf",
                                command, false );
        TCLAP::SwitchArg objectsArg( "o", "object",
                   "Benchmark object-object instead of node-node communication",
                                     command, false );
        TCLAP::ValueArg<size_t> sizeArg( "p", "packetSize", "packet size",
                                         false, packetSize, "unsigned",
                                         command );
        TCLAP::ValueArg<size_t> packetsArg( "n", "numPackets",
                                            "number of packets to send",
                                            false, nPackets, "unsigned",
                                            command );
        TCLAP::ValueArg<uint32_t> waitArg( "w", "wait",
                                           "wait time (ms) between sends",
                                           false, 0, "unsigned", command );
        command.parse( argc, argv );

        if( remoteArg.isSet( ))
        {
            remote = new co::ConnectionDescription;
            remote->port = 4242;
            remote->fromString( remoteArg.getValue( ));
        }
        useZeroconf = !zcArg.isSet();
        useObjects = objectsArg.isSet();

        if( sizeArg.isSet( ))
            packetSize = sizeArg.getValue();
        if( packetsArg.isSet( ))
            nPackets = uint32_t( packetsArg.getValue( ));
        if( waitArg.isSet( ))
            waitTime = waitArg.getValue();
    }
    catch( TCLAP::ArgException& exception )
    {
        LBERROR << "Command line parse error: " << exception.error()
                << " for argument " << exception.argId() << std::endl;

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
    object.setID( _objectID + localNode->getNodeID( ));
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
                localNode->connect( co::uint128_t( instance ));
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
                                               _objectID + peer->getNodeID( )));
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
