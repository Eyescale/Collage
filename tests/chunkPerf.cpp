
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

// Tests tradeoff between memcpy to aggregate or system call to transmit data

#include <test.h>

#include <co/buffer.h>
#include <co/connection.h>
#include <co/connectionDescription.h>
#include <co/init.h>
#include <co/version.h>
#include <lunchbox/clock.h>

#pragma warning( disable: 4275 )
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#pragma warning( default: 4275 )
namespace arg = boost::program_options;

static const size_t maxBufferSize = LB_64MB;
static const size_t minChunkSize  = 512;

class Server : public lunchbox::Thread
{
public:
    Server( co::ConnectionDescriptionPtr description )
        : connection_( co::Connection::create( description ))
    {}

private:
    bool init() override
    {
        if( !connection_ || !connection_->listen( ))
            return false;

        connection_->acceptNB();
        return true;
    }

    void run() override
    {
        co::Buffer buffer;
        co::BufferPtr syncBuffer;
        buffer.resize( maxBufferSize );

        co::ConnectionPtr client = connection_->acceptSync();
        for( size_t size = minChunkSize; size <= maxBufferSize; size = size<<1 )
        {
            buffer.setSize( 0 );
            client->recvNB( &buffer, maxBufferSize );
            client->recvSync( syncBuffer, maxBufferSize );
            TEST( syncBuffer == &buffer );

            buffer.setSize( 0 );
            client->recvNB( &buffer, maxBufferSize );
            client->recvSync( syncBuffer, maxBufferSize );
            TEST( syncBuffer == &buffer );
        }

        client->close();
        connection_->close();
    }

    co::ConnectionPtr connection_;
};

class Client
{
public:
    void run( co::ConnectionDescriptionPtr description )
    {
        co::ConnectionPtr connection = co::Connection::create( description );
        TEST( connection->connect( ));

        co::Buffer buffer;
        buffer.resize( maxBufferSize );

        co::Buffer input;
        input.resize( maxBufferSize );
        input.setZero();

        lunchbox::Clock clock;

        for( size_t size = minChunkSize; size <= maxBufferSize; size = size<<1 )
        {
            clock.reset();
            for( size_t i = 0; i < maxBufferSize; i += size )
                memcpy( buffer.getData() + i, input.getData() + i, size );

            TEST( connection->send( buffer.getData(), buffer.getSize( )));
            const float aggTime = clock.resetTimef();

            //-----
            connection->lockSend();
            for( size_t i = 0; i < maxBufferSize; i += size )
                TEST( connection->send( buffer.getData() + i, size, true ));
            connection->unlockSend();
            const float sepTime = clock.getTimef();

            std::cout << "Chunk size " << std::setw(9) << size << " aggregated "
                      << aggTime << " ms, separate " << sepTime << " ms"
                      << std::endl;
        }

        connection->close();
    }
};

int main( const int argc, char* argv[] )
{
    const std::string applicationName = "send vs memcpy performance test";
    arg::variables_map vm;
    arg::options_description desc( applicationName );
    std::string param;

    desc.add_options()
        ( "help", "output this help message" )
        ( "version,v", "print version" )
        ( "server,s", arg::value< std::string >( &param ),
          "run as server and wait for client connection" )
        ( "client,c", arg::value< std::string >( &param ),
          "connect to a chunkPerf server using the given connection" );
    try
    {
        arg::store( arg::parse_command_line( argc, argv, desc ), vm );
        arg::notify( vm );
    }
    catch( ... )
    {
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }

    if( vm.count( "help" ))
    {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }
    if( vm.count( "version" ))
    {
        std::cout << applicationName << " " << co::Version::getString()
                  << std::endl;
        return EXIT_SUCCESS;
    }

    TEST( co::init( argc, argv ));

    const bool isServer = vm.count( "server" );
    const bool isClient = !isServer && vm.count( "client" );

    co::ConnectionDescriptionPtr description = new co::ConnectionDescription;
    if( isServer )
        TEST( description->fromString( param ));
    if( isClient )
        TEST( description->fromString( param ));
    TESTINFO( param.empty(), param );

    Server server( description );
    if( !isClient ) // start server thread
        TEST( server.start( ));

    if( !isServer ) // run client code
    {
        Client client;
        client.run( description );
    }

    if( !isClient ) // wait for server thread
        TEST( server.join( ));

    TEST( co::exit( ));
    return EXIT_SUCCESS;
}
