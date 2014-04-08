
/* Copyright (c) 2013, Stefan.Eilemann@epfl.ch
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

#define TEST_NO_WATCHDOG
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

static const std::string payload( "Hi! I am your payload" );

class LocalNode : public co::LocalNode
{
    typedef co::CommandFunc< LocalNode > CmdFunc;

public:
    LocalNode()
        : gotAsync_( false )
        , counter_( 0 )
    {
        co::CommandQueue* q = getCommandThreadQueue();
        registerCommand( CMD_ASYNC, CmdFunc( this, &LocalNode::_cmdAsync ), q );
        registerCommand( CMD_SYNC, CmdFunc( this, &LocalNode::_cmdSync ), q );
        registerCommand( CMD_DATA, CmdFunc( this, &LocalNode::_cmdData ), q );
        registerCommand( CMD_DATA_REPLY,
                         CmdFunc( this, &LocalNode::_cmdDataReply ), q );
    }

private:
    bool gotAsync_;
    uint32_t counter_;

    bool _cmdAsync( co::ICommand& )
    {
        gotAsync_ = true;
        return true;
    }

    bool _cmdSync( co::ICommand& command )
    {
        TEST( gotAsync_ );
        ackRequest( command.getNode(), command.get< uint32_t >( ));
        return true;
    }

    bool _cmdData( co::ICommand& command )
    {
        TEST( gotAsync_ );
        command.getNode()->send( CMD_DATA_REPLY )
            << command.get< uint32_t >() << ++counter_;
        TEST( command.get< std::string >() == payload );
        return true;
    }

    bool _cmdDataReply( co::ICommand& command )
    {
        TEST( !gotAsync_ );
        const uint32_t request = command.get< uint32_t >();
        const uint32_t result = command.get< uint32_t >();
        TEST( result == ++counter_ );
        serveRequest( request, result );
        return true;
    }
};

typedef lunchbox::RefPtr< LocalNode > LocalNodePtr;

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

    lunchbox::Request< void > request = client->registerRequest< void >();
    serverProxy->send( CMD_ASYNC );
    serverProxy->send( CMD_SYNC ) << request.getID();
    request.wait();

    lunchbox::Clock clock;
    for( size_t i = 0; i < NCOMMANDS; ++i )
    {
        lunchbox::Request< void > future = client->registerRequest< void >();
        serverProxy->send( CMD_SYNC ) << future.getID();
    }
    const float syncTimeFuture = clock.resetTimef();

    for( size_t i = 0; i < NCOMMANDS; ++i )
    {
        lunchbox::Request< uint32_t > future =
            client->registerRequest< uint32_t >();
        serverProxy->send( CMD_SYNC ) << future.getID();
    }
    const float syncTime = clock.resetTimef();

    serverProxy->send( CMD_SYNC ) << request;
    client->waitRequest( request.getID() );
    const float asyncTime = clock.resetTimef();

    std::cout << "Async command: " << asyncTime/float(NCOMMANDS)
              << " ms, sync: " << syncTime/float(NCOMMANDS)
              << std::endl;

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
