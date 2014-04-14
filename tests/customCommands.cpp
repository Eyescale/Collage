
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.ch>
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

#include <co/defines.h>
#include <test.h>

#include <co/co.h>
#include <lunchbox/monitor.h>
#include <lunchbox/rng.h>
#include <boost/bind.hpp>

const co::uint128_t cmdID1( lunchbox::make_uint128( "ch.eyescale.collage.test.c1" ));
const co::uint128_t cmdID2( lunchbox::make_uint128( "ch.eyescale.collage.test.c2" ));
lunchbox::Monitor<bool> gotCmd1;
lunchbox::Monitor<bool> gotCmd2;

class MyLocalNode : public co::LocalNode
{
public:
    bool cmdCustom1( co::CustomICommand& command )
    {
        TEST( command.getCommandID() == cmdID1 );
        gotCmd1 = true;
        return true;
    }

    bool cmdCustom2( co::CustomICommand& command )
    {
        TEST( command.getCommandID() == cmdID2 );
        gotCmd2 = true;
        TEST( command.get< std::string >() == "hello" );
        return true;
    }
};

typedef lunchbox::RefPtr< MyLocalNode > MyLocalNodePtr;

int main( int argc, char **argv )
{
    TEST( co::init( argc, argv ) );

    co::ConnectionDescriptionPtr connDesc = new co::ConnectionDescription;

    lunchbox::RNG rng;
    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->port = (rng.get<uint16_t>() % 60000) + 1024;
    connDesc->setHostname( "localhost" );

    MyLocalNodePtr server = new MyLocalNode;
    server->addConnectionDescription( connDesc );
    TEST( server->listen( ));

    co::NodePtr serverProxy = new co::Node;
    serverProxy->addConnectionDescription( connDesc );

    connDesc = new co::ConnectionDescription;
    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->setHostname( "localhost" );

    co::LocalNodePtr client = new co::LocalNode;
    client->addConnectionDescription( connDesc );
    TEST( client->listen( ));
    TEST( client->connect( serverProxy ));

    server->registerCommandHandler( cmdID1,
                                    boost::bind( &MyLocalNode::cmdCustom1,
                                                 server.get(), _1 ),
                                    server->getCommandThreadQueue( ));
    server->registerCommandHandler( cmdID2,
                                    boost::bind( &MyLocalNode::cmdCustom2,
                                                 server.get(), _1 ), 0 );

    serverProxy->send( cmdID1 );
    serverProxy->send( cmdID2 ) << std::string( "hello" );

    TEST( gotCmd1.timedWaitEQ( true, 1000 ));
    TEST( gotCmd2.timedWaitEQ( true, 1000 ));

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
