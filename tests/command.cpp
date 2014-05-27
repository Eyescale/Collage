
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

#include <co/co.h>
#include <test.h>
#include <lunchbox/clock.h>
#include <lunchbox/rng.h>

#define NCOMMANDS 10000

enum Commands
{
    CMD_ASYNC = co::CMD_NODE_CUSTOM,
    CMD_SYNC,
    CMD_DATA,
    CMD_DATA_REPLY
};

static const std::string payload( "Hi! I am your payload. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut eget felis sed leo tincidunt dictum eu eu felis. Aenean aliquam augue nec elit tristique tempus. Pellentesque dignissim adipiscing tellus, ut porttitor nisl lacinia vel. Donec malesuada lobortis velit, nec lobortis metus consequat ac. Ut dictum rutrum dui. Pellentesque quis risus at lectus bibendum laoreet. Suspendisse tristique urna quis urna faucibus et auctor risus ultricies. Morbi vitae mi vitae nisi adipiscing ultricies ac in nulla. Nam mattis venenatis nulla, non posuere felis tempus eget. Cras dapibus ultrices arcu vel dapibus. Nam hendrerit lacinia consectetur. Donec ullamcorper nibh nisl, id aliquam nisl. Nunc at tortor a lacus tincidunt gravida vitae nec risus. Suspendisse potenti. Fusce tristique dapibus ipsum, sit amet posuere turpis fermentum nec. Nam nec ante dolor." );

class LocalNode : public co::LocalNode
{
    typedef co::CommandFunc< LocalNode > CmdFunc;

public:
    LocalNode()
        : _gotAsync( false )
        , _counter( 0 )
    {
        co::CommandQueue* q = getCommandThreadQueue();
        registerCommand( CMD_ASYNC, CmdFunc( this, &LocalNode::_cmdAsync ), q );
        registerCommand( CMD_SYNC, CmdFunc( this, &LocalNode::_cmdSync ), q );
        registerCommand( CMD_DATA, CmdFunc( this, &LocalNode::_cmdData ), q );
        registerCommand( CMD_DATA_REPLY,
                         CmdFunc( this, &LocalNode::_cmdDataReply ), q );
    }

private:
    bool _gotAsync;
    uint32_t _counter;

    bool _cmdAsync( co::ICommand& )
    {
        _gotAsync = true;
        return true;
    }

    bool _cmdSync( co::ICommand& command )
    {
        TEST( _gotAsync );
        ackRequest( command.getNode(), command.get< uint32_t >( ));
        return true;
    }

    bool _cmdData( co::ICommand& command )
    {
        TEST( _gotAsync );
        command.getNode()->send( CMD_DATA_REPLY )
            << command.get< uint32_t >() << ++_counter;
        TEST( command.get< std::string >() == payload );
        return true;
    }

    bool _cmdDataReply( co::ICommand& command )
    {
        TEST( !_gotAsync );
        const uint32_t request = command.get< uint32_t >();
        const uint32_t result = command.get< uint32_t >();
        TEST( result == ++_counter );
        serveRequest( request, result );
        return true;
    }
};

typedef lunchbox::RefPtr< LocalNode > LocalNodePtr;

#ifndef _WIN32
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

int main( int argc, char **argv )
{
    TEST( co::init( argc, argv ) );

    co::ConnectionDescriptionPtr connDesc = new co::ConnectionDescription;

    lunchbox::RNG rng;
    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->port = (rng.get<uint16_t>() % 60000) + 1024;
    connDesc->setHostname( "localhost" );

    LocalNodePtr server = new LocalNode;
    server->addConnectionDescription( connDesc );
    TEST( server->listen( ));

    co::NodePtr serverProxy = new co::Node;
    serverProxy->addConnectionDescription( connDesc );

    connDesc = new co::ConnectionDescription;
    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->setHostname( "localhost" );

    LocalNodePtr client = new LocalNode;
    client->addConnectionDescription( connDesc );
    TEST( client->listen( ));
    TEST( client->connect( serverProxy ));

    lunchbox::Clock clock;
    for( size_t i = 0; i < NCOMMANDS; ++i )
        serverProxy->send( CMD_ASYNC );
    uint32_t request = client->registerRequest();
    serverProxy->send( CMD_SYNC ) << request;
    client->waitRequest( request );
    const float asyncTime = clock.resetTimef();

    for( size_t i = 0; i < NCOMMANDS; ++i )
    {
        request = client->registerRequest();
        serverProxy->send( CMD_SYNC ) << request;
        client->waitRequest( request );
    }
    const float syncTime = clock.resetTimef();

    for( size_t i = 0; i < NCOMMANDS; ++i )
    {
        lunchbox::Request< void > future = client->registerRequest< void >();
        serverProxy->send( CMD_SYNC ) << future;
    }
    const float syncTimeFuture = clock.resetTimef();

    for( size_t i = 0; i < NCOMMANDS; ++i )
    {
        lunchbox::Request< uint32_t > future =
            client->registerRequest< uint32_t >();
        serverProxy->send( CMD_DATA ) << future << payload;
    }
    const float syncTimePayload = clock.resetTimef();

    std::cout << "Async command: " << asyncTime/float(NCOMMANDS)
              << " ms, sync: " << syncTime/float(NCOMMANDS)
              << " ms, using future: " << syncTimeFuture/float(NCOMMANDS+1)
              << " ms, with " << payload.size() << "b payload: "
              << syncTimePayload/float(NCOMMANDS+1) << std::endl;

    TEST( client->disconnect( serverProxy ));
    TEST( client->close( ));
    TEST( server->close( ));

    serverProxy->printHolders( std::cerr );
    TESTINFO( serverProxy->getRefCount() == 1, serverProxy->getRefCount( ));
    TESTINFO( client->getRefCount() == 1, client->getRefCount( ));
    TESTINFO( server->getRefCount() == 1, server->getRefCount( ));

    serverProxy = 0;
    client      = 0;
    server      = 0;

    TEST( co::exit( ));

    return EXIT_SUCCESS;
}
