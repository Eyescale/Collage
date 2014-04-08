
/* Copyright (c) 2013, Daniel Nachbaur <daniel.nachbaur@epfl.ch>
 *               2013, Stefan.Eilemann@epfl.ch
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
#include <lunchbox/rng.h>

namespace
{

class TestObject : public co::Object
{
public:
    TestObject() {}

    std::string message;

protected:
    virtual void getInstanceData( co::DataOStream& os )
        { os << message; }

    virtual void applyInstanceData( co::DataIStream& is )
        { is >> message; }

    virtual ChangeType getChangeType() const { return INSTANCE; }
};

typedef TestObject Foo;
typedef TestObject Bar;

enum ObjectType
{
    TYPE_FOO = co::OBJECTTYPE_CUSTOM,
    TYPE_BAR
};

static Foo* clientFoo = 0;

class ObjectFactory : public co::ObjectFactory
{
public:
    virtual co::Object* createObject( const uint32_t type )
    {
        switch( type )
        {
        case TYPE_FOO:
            return new Foo;
        case TYPE_BAR:
            return new Bar;
        default:
            return 0;
        }
    }

    virtual void destroyObject( co::Object* object, const uint32_t type )
    {
        TEST( type == TYPE_FOO );
        TEST( object == clientFoo );
        delete object;
    }
};

class TestNode : public co::LocalNode
{
public:
#pragma warning( disable: 4355)
    TestNode() : factory(), objectMap( *this, factory ) {}
#pragma warning( default: 4355)

    ObjectFactory factory;
    co::ObjectMap objectMap;
};
}

int main( int argc, char **argv )
{
    TEST( co::init( argc, argv ));

    lunchbox::RNG rng;
    const uint16_t port = (rng.get<uint16_t>() % 60000) + 1024;

    lunchbox::RefPtr< TestNode > server = new TestNode;
    co::ConnectionDescriptionPtr connDesc =
        new co::ConnectionDescription;

    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->port = port;
    connDesc->setHostname( "localhost" );

    server->addConnectionDescription( connDesc );
    TEST( server->listen( ));

    co::NodePtr serverProxy = new co::Node;
    serverProxy->addConnectionDescription( connDesc );

    connDesc = new co::ConnectionDescription;
    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->setHostname( "localhost" );

    {
        lunchbox::RefPtr< TestNode > client = new TestNode;
        client->addConnectionDescription( connDesc );
        TEST( client->listen( ));
        TEST( client->connect( serverProxy ));

        // init
        TEST( server->registerObject( &server->objectMap ));
        TEST( client->mapObject( &client->objectMap, &server->objectMap ));


        // Test register()
        Foo masterFoo;
        masterFoo.message = "hello foo";
        TEST( server->objectMap.register_( &masterFoo, TYPE_FOO ));
        TEST( !server->objectMap.register_( &masterFoo, TYPE_FOO ));

        Bar masterBar;
        masterBar.message = "hello bar";
        TEST( server->objectMap.register_( &masterBar, TYPE_BAR ));

        // Test map()
        client->objectMap.sync( server->objectMap.commit( ));
        clientFoo = static_cast< Foo* >
                                  ( client->objectMap.map( masterFoo.getID( )));
        TEST( clientFoo->message == "hello foo" );

        // Test map() of instance
        Bar clientBar;
        TEST( client->objectMap.map( masterBar.getID(),
                                     &clientBar ) == &clientBar );
        TEST( clientBar.message == "hello bar" );

        // Test sync()
        masterBar.message = "hello again";
        client->objectMap.sync( server->objectMap.commit( ));
        TEST( clientBar.message == "hello again" );

        // Test deregister()
        TEST( server->objectMap.deregister( &masterBar ));
        masterBar.message = "still there?";
        client->objectMap.sync( server->objectMap.commit( ));
        TEST( clientBar.message == "hello again" );

        // Test unmap()
        TEST( client->objectMap.unmap( clientFoo ));

        // Test deregister()
        TEST( server->objectMap.deregister( &masterFoo ));
        TEST( !server->objectMap.deregister( &masterFoo ));

        // exit
        client->objectMap.clear();
        client->unmapObject( &client->objectMap );
        server->deregisterObject( &server->objectMap );

        TEST( client->disconnect( serverProxy ));
        TEST( client->close( ));
    }
    TEST( server->close( ));

    serverProxy = 0;
    server      = 0;

    co::exit();
    return EXIT_SUCCESS;
}
