
/* Copyright (c) 2012, Stefan.Eilemann@epfl.ch
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
typedef lunchbox::Lockable< co::Nodes, lunchbox::SpinLock > ConnectedNodes;

ConnectedNodes nodes_;
lunchbox::Lock print_;

class PerfNode : public co::LocalNode
{
public:
    PerfNode() : co::LocalNode( 0xC0FFEEu ) {}

private:
    virtual co::NodePtr createNode( const uint32_t type )
        { return new co::Node( type ); }

    virtual void notifyConnect( co::NodePtr node )
    {
        lunchbox::ScopedFastWrite _mutex( nodes_ );
        nodes_->push_back( node );
    }

    virtual void notifyDisconnect( co::NodePtr node )
    {
        lunchbox::ScopedFastWrite _mutex( nodes_ );
        co::NodesIter i = std::find( nodes_->begin(), nodes_->end(), node );
        if( i != nodes_->end( ))
            nodes_->erase( i );
    }
};

}

int main( int argc, char **argv )
{
    if( !co::init( argc, argv ))
        return EXIT_FAILURE;

    co::ConnectionDescriptionPtr remote;
    size_t packetSize = 1048576u; // needs to be modulo 8
    uint32_t nPackets   = 0xFFFFFFFFu;
    uint32_t waitTime = 0;
    bool useZeroconf = true;

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

        if( sizeArg.isSet( ))
            packetSize = sizeArg.getValue();
        if( packetsArg.isSet( ))
            nPackets = packetsArg.getValue();
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
    co::LocalNodePtr localNode = new co::LocalNode;
    if( !localNode->initLocal( argc, argv ))
    {
        co::exit();
        return EXIT_FAILURE;
    }
    localNode->getZeroconf().set( "coNodeperf", co::Version::getString( ));

    // run
    if( remote )
    {
        co::NodePtr node = new co::Node;
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

    while( true )
    {
        lunchbox::Thread::yield();

        lunchbox::ScopedFastRead _mutex( nodes_ );
        if( !nodes_->empty( ))
            break;
    }

    lunchbox::Buffer< uint64_t > buffer;
    const size_t bufferElems = packetSize / sizeof( uint64_t );
    buffer.resize( bufferElems );
    for( size_t i = 0; i < bufferElems; ++i )
        buffer[i] = i;

    const float mBytesSec = buffer.getSize() / 1024.0f / 1024.0f * 1000.0f;
    lunchbox::Clock clock;
    size_t sentPackets = 0;

    clock.reset();
    while( nPackets-- )
    {
        co::Nodes nodes;
        {
            lunchbox::ScopedFastRead _mutex( nodes_ );
            nodes = *nodes_;
        }
        if( nodes.empty( ))
            break;

        for( co::NodesCIter i = nodes.begin(); i != nodes.end(); ++i )
        {
            co::NodePtr node = *i;
            if( node->getType() != 0xC0FFEEu )
                continue;

            node->send( co::CMD_NODE_CUSTOM ) << nPackets << buffer;
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

    LBCHECK( co::exit( ));
    return EXIT_SUCCESS;
}

