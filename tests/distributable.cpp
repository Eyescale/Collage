
/* Copyright (c) 2015-2016, Daniel.Nachbaur@epfl.ch
 *                          Stefan.Eilemann@epfl.ch
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

class CameraBase;
typedef co::Distributable< CameraBase > Camera;
typedef std::array< float, 3 > Vector3f;
Vector3f lookAtVector{{ 1, 2, 3 }};
Vector3f upVector{{ -1, 0, 0 }};

lunchbox::Monitorb received( false );

class CameraBase : public servus::Serializable
{
public:
    std::string getTypeName() const final { return "co::test::CameraBase"; }

    void setLookAt( const Vector3f& lookAt )
        { notifyChanging(); _lookAt = lookAt; }
    void setUp( const Vector3f& up ) { notifyChanging(); _up = up; }

    bool operator == ( const CameraBase& rhs ) const
        { return _lookAt == rhs._lookAt && _up == rhs._up; }
    bool operator != ( const CameraBase& rhs ) const
        { return _lookAt != rhs._lookAt || _up != rhs._up; }

protected:
    virtual void notifyChanging() = 0;

private:
    bool _fromBinary( const void* data, const size_t size ) final
    {
        LBASSERT( size == 2 * sizeof( Vector3f ));
        ::memcpy( &_lookAt, data, size );
        return true;
    }

    Data _toBinary() const final
    {
        Data data;
        data.ptr = std::shared_ptr< const void >( &_lookAt,
                                                  []( const void* ){} );
        data.size = 2 * sizeof( Vector3f );
        return data;
    }

    Vector3f _lookAt;
    Vector3f _up;
};

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
        object = new Camera;
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

        Camera camera;
        camera.setLookAt( lookAtVector );
        TEST( client->registerObject( &camera ));
        camera.push( co::uint128_t(42), co::uint128_t(42), nodes );

        received.waitEQ( true );
        TEST( server->mapObject( server->object, camera.getID( )));
        TEST( camera == static_cast< const Camera& >( *server->object ));

        camera.setUp( upVector );
        TEST( camera != static_cast< const Camera& >( *server->object ));

        server->object->sync( camera.commit( ));
        TEST( camera == static_cast< const Camera& >( *server->object ));

        server->unmapObject( server->object );
        delete server->object;
        server->object = 0;

        client->deregisterObject( &camera );
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
