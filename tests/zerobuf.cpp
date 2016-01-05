
/* Copyright (c) 2015, Daniel.Nachbaur@epfl.ch
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

#include <co/co.h>
#include <lunchbox/monitor.h>
#include <zerobuf/render/render.h>

typedef co::ZeroBuf< zerobuf::render::Camera > TestObject;
zerobuf::render::Vector3f lookAtVector( { 1, 2, 3 } );
zerobuf::render::Vector3f upVector( { -1, 0, 0 } );

lunchbox::Monitorb received( false );

class Server : public co::LocalNode
{
public:
    Server() : object( 0 ) {}

    co::Object* object;

protected:
    void objectPush( const co::uint128_t& /*groupID*/,
                     const co::uint128_t& /*typeID*/,
                     const co::uint128_t& /*objectID*/, co::DataIStream& is )
        override
    {
        TEST( !object );
        object = new TestObject;
        object->applyInstanceData( is );
        TESTINFO( !is.hasData(), is.nRemainingBuffers( ));
        received = true;
    }
};

int main( int argc, char **argv )
{
    co::init( argc, argv );
    co::Global::setObjectBufferSize( 600 );

    lunchbox::RefPtr< Server > server = new Server;
    co::ConnectionDescriptionPtr connDesc =
        new co::ConnectionDescription;

    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->setHostname( "localhost" );

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

    {
        co::Nodes nodes;
        nodes.push_back( serverProxy );

        TestObject object;
        object.setLookAt( lookAtVector );
        TEST( client->registerObject( &object ));
        object.push( co::uint128_t(42), co::uint128_t(42), nodes );

        received.waitEQ( true );
        TEST( server->mapObject( server->object, object.getID( )));
        TEST( object == static_cast< const TestObject&>( *server->object ));

        object.setUp( upVector );
        TEST( object != static_cast< const TestObject&>( *server->object ));

        server->object->sync( object.commit( ));
        TEST( object == static_cast< const TestObject&>( *server->object ));

        server->unmapObject( server->object );
        delete server->object;
        server->object = 0;

        client->deregisterObject( &object );
    }

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
