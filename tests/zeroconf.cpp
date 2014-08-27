
/* Copyright (c) 2012-2014, Stefan Eilemann <eile@eyescale.ch>
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

#include <test.h>

#include <co/connection.h>
#include <co/connectionDescription.h>
#include <co/init.h>
#include <co/localNode.h>
#include <co/zeroconf.h>

#include <iostream>

using co::uint128_t;

#ifdef COLLAGE_USE_SERVUS
int main( int argc, char **argv )
{
    co::init( argc, argv );

    co::ConnectionDescriptionPtr connDesc = new co::ConnectionDescription;
    connDesc->setHostname( "localhost" );

    co::LocalNodePtr server = new co::LocalNode;
    server->addConnectionDescription( connDesc );
    TEST( server->listen( ));

    co::LocalNodePtr client = new co::LocalNode;
    TEST( client->listen( ));

    co::Zeroconf zeroconf = client->getZeroconf();
    if( zeroconf.getInstances().empty() && getenv( "TRAVIS" ))
    {
        std::cerr << "Bailing, got no hosts on a Travis CI setup" << std::endl;
        client->close();
        server->close();
        co::exit();
        return EXIT_SUCCESS;
    }

    co::NodePtr serverProxy = client->connect( server->getNodeID( ));
    TEST( serverProxy );

    zeroconf = server->getZeroconf();
    zeroconf.set( "co_test_value", "42" );
    lunchbox::sleep( 500 /*ms*/ ); // give it time to propagate
    zeroconf = client->getZeroconf(); // rediscover, use other peer for a change

    const co::Strings& instances = zeroconf.getInstances();
    bool found = false;
    TEST( instances.size() >= 1 );

    for( co::StringsCIter i = instances.begin(); i != instances.end(); ++i )
    {
        const std::string& instance = *i;
        const uint128_t nodeID( instance );
        TEST( nodeID != 0 );
        TEST( nodeID != client->getNodeID( ));

        if( nodeID != server->getNodeID( ))
            continue;

        TEST( zeroconf.get( instance, "co_numPorts" ) == "1" );
        TESTINFO( zeroconf.get( instance, "co_test_value" ) == "42",
                  "Instance #" << i - instances.begin() << ": " <<
                  zeroconf.get( instance, "co_test_value" ));
        found = true;
    }

    TEST( found );
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

    co::exit();
    return EXIT_SUCCESS;
}
#else
int main( int, char ** )
{
    return EXIT_SUCCESS;
}
#endif
