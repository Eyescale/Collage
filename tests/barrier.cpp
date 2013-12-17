
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
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

#include <co/barrier.h>
#include <co/connection.h>
#include <co/connectionDescription.h>
#include <co/init.h>
#include <co/node.h>
#include <lunchbox/monitor.h>
#include <lunchbox/rng.h>

#include <iostream>

lunchbox::Monitor< co::Barrier* > _barrier( 0 );
static uint16_t _port = 0;

class NodeThread : public lunchbox::Thread
{
public:
    NodeThread( const bool master ) : _master(master) {}

    virtual void run()
    {
        co::ConnectionDescriptionPtr description =
            new co::ConnectionDescription;
        description->type = co::CONNECTIONTYPE_TCPIP;
        description->port = _master ? _port : _port+1;

        co::LocalNodePtr node = new co::LocalNode;
        node->addConnectionDescription( description );
        TEST( node->listen( ));

        if( _master )
        {
            co::Barrier barrier( node, node->getNodeID(), 3 );
            TEST( barrier.isAttached( ));
            TEST( barrier.getVersion() == co::VERSION_FIRST );
            TEST( barrier.getHeight() ==  3 );

            _barrier = &barrier;
            barrier.enter();

            barrier.setHeight( 2 );
            barrier.commit();
            TEST( barrier.getVersion() == co::VERSION_FIRST + 1 );

            barrier.enter();
            _barrier.waitEQ( 0 ); // wait for slave thread finish
            node->deregisterObject( &barrier );
        }
        else
        {
            co::NodePtr server = new co::Node;
            co::ConnectionDescriptionPtr serverDesc =
                new co::ConnectionDescription;
            serverDesc->port = _port;
            server->addConnectionDescription( serverDesc );

            _barrier.waitNE( 0 );
            TEST( node->connect( server ));

            co::Barrier barrier( node, _barrier.get( ));
            TEST( barrier.isGood( ));
            TEST( barrier.getVersion() == co::VERSION_FIRST );

            std::cerr << "Slave enter" << std::endl;
            barrier.enter();
            std::cerr << "Slave left" << std::endl;

            barrier.sync( co::VERSION_FIRST + 1 );
            TEST( barrier.getVersion() == co::VERSION_FIRST + 1 );

            std::cerr << "Slave enter" << std::endl;
            barrier.enter();
            std::cerr << "Slave left" << std::endl;

            node->unmapObject( &barrier );
        }

        node->close();
    }

private:
    bool _master;
};

int main( int argc, char **argv )
{
    TEST( co::init( argc, argv ));
    lunchbox::RNG rng;
    _port =(rng.get<uint16_t>() % 60000) + 1024;

    NodeThread server( true );
    NodeThread node( false );

    server.start();
    node.start();

    _barrier.waitNE( 0 );
    std::cerr << "Main enter" << std::endl;
    _barrier->enter();
    std::cerr << "Main left" << std::endl;
    _barrier = 0;

    node.join();
    _barrier = 0;

    server.join();

    co::exit();
    return EXIT_SUCCESS;
}
