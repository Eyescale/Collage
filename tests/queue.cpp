
/* Copyright (c) 2011, Stefan Eilemann <eile@equalizergraphics.com> 
 *               2011, Carsten Rohn <carsten.rohn@rtt.ag> 
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

#include <co/command.h>
#include <co/connectionDescription.h>
#include <co/init.h>
#include <co/node.h>
#include <co/objectICommand.h>
#include <co/queueItem.h>
#include <co/queueMaster.h>
#include <co/queueSlave.h>
#include <lunchbox/sleep.h>


int main( int argc, char **argv )
{
    TEST( co::init( argc, argv ));

    co::LocalNodePtr node = new co::LocalNode;
    node->initLocal(argc, argv);

    co::QueueMaster* qm = new co::QueueMaster;
    co::QueueSlave* qs = new co::QueueSlave;

    node->registerObject(qm);
    node->mapObject(qs, qm->getID(), co::VERSION_FIRST);

    qm->push();
    qm->push() << 42u;
    qm->push() << std::string( "hallo" );
    qm->push() << 1.5f << false << co::VERSION_FIRST;

    {
        co::CommandPtr c1 = qs->pop();
        co::CommandPtr c2 = qs->pop();
        co::CommandPtr c3 = qs->pop();
        co::CommandPtr c4 = qs->pop();
        co::CommandPtr c5 = qs->pop();

        TEST( c1 );
        TEST( c2 );
        co::ObjectICommand c2cmd( c2 );
        TEST( c2cmd.get< uint32_t >() == 42 )
        TEST( c3 );
        co::ObjectICommand c3cmd( c3 );
        TEST( c3cmd.get< std::string >() == "hallo" )
        TEST( c4 );
        co::ObjectICommand c4cmd( c4 );
        TEST( c4cmd.get< float >() == 1.5f )
        TEST( c4cmd.get< bool >() == false )
        TEST( c4cmd.get< co::uint128_t >() == co::VERSION_FIRST )
        TEST( !c5 );
    }

    lunchbox::sleep(10);

    node->unmapObject( qs );
    node->deregisterObject( qm );

    delete qs;
    delete qm;

    node->close();

    co::exit();
    return EXIT_SUCCESS;
}

