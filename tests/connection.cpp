
/* Copyright (c) 2010-2014, Stefan Eilemann <eile@equalizergraphics.com>
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

// Tests basic connection functionality
#include <test.h>
#include <co/buffer.h>
#include <co/connection.h>
#include <co/connectionDescription.h>
#include <co/connectionSet.h>
#include <co/init.h>

#include <lunchbox/clock.h>
#include <lunchbox/monitor.h>
#include <iostream>

#define PACKETSIZE (12345)
#define NPACKETS   (23456)

namespace
{
static co::ConnectionType types[] =
{
    co::CONNECTIONTYPE_TCPIP,
    co::CONNECTIONTYPE_PIPE,
    co::CONNECTIONTYPE_NAMEDPIPE,
    co::CONNECTIONTYPE_RSP,
    co::CONNECTIONTYPE_RDMA,
//    co::CONNECTIONTYPE_UDT,
    co::CONNECTIONTYPE_NONE // must be last
};

class Reader : public lunchbox::Thread
{
public:
    Reader( co::ConnectionPtr connection ) : connection_( connection )
    {
        TEST( start( ));
    }

    void run() override
    {
        co::Buffer buffer;
        co::BufferPtr syncBuffer;

        for( size_t i = 0; i < NPACKETS; ++i )
        {
            connection_->recvNB( &buffer, PACKETSIZE );
            TEST( connection_->recvSync( syncBuffer ));
            TEST( syncBuffer == &buffer );
            TEST( buffer.getSize() == PACKETSIZE );
            buffer.setSize( 0 );
        }

        connection_->recvNB( &buffer, PACKETSIZE );
        TEST( !connection_->recvSync( syncBuffer ));
        TEST( connection_->isClosed( ));
        connection_ = 0;
    }

private:
    co::ConnectionPtr connection_;
};

}

int main( int argc, char **argv )
{
    co::init( argc, argv );

    for( size_t i = 0; types[i] != co::CONNECTIONTYPE_NONE; ++i )
    {
        co::ConnectionDescriptionPtr desc = new co::ConnectionDescription;
        desc->type = types[i];

        if( desc->type >= co::CONNECTIONTYPE_MULTICAST )
            desc->setHostname( "239.255.12.34" );
        else
            desc->setHostname( "127.0.0.1" );

        co::ConnectionPtr listener = co::Connection::create( desc );
        if( !listener )
        {
            std::cout << desc->type << ": not supported" << std::endl;
            continue;
        }

        co::ConnectionPtr writer;
        co::ConnectionPtr reader;

        switch( desc->type ) // different connections, different semantics...
        {
        case co::CONNECTIONTYPE_PIPE:
            writer = listener;
            TEST( writer->connect( ));
            reader = writer->acceptSync();
            break;

        case co::CONNECTIONTYPE_RSP:
            TESTINFO( listener->listen(), desc );
            listener->acceptNB();

            writer = listener;
            reader = listener->acceptSync();
            break;

        default:
        {
            const bool listening = listener->listen();
            if( !listening && desc->type == co::CONNECTIONTYPE_RDMA )
                continue; // No local IB adapter up

            TESTINFO( listening, desc );
            listener->acceptNB();

            writer = co::Connection::create( desc );
            TEST( writer->connect( ));

            reader = listener->acceptSync();
            break;
        }
        }

        TEST( writer );
        TEST( reader );

        Reader readThread( reader );
        uint8_t out[ PACKETSIZE ];

        lunchbox::Clock clock;
        for( size_t j = 0; j < NPACKETS; ++j )
            TEST( writer->send( out, PACKETSIZE ));

        writer->close();
        readThread.join();
        const float time = clock.getTimef();

        std::cout << desc->type << ": "
                  << NPACKETS * PACKETSIZE / 1024.f / 1024.f * 1000.f / time
                  << " MB/s" << std::endl;

        if( listener == writer )
            listener = 0;
        if( reader == writer )
            reader = 0;

        if( listener.isValid( ))
            TEST( listener->getRefCount() == 1 );
        if( reader.isValid( ))
            TEST( reader->getRefCount() == 1 );
        TEST( writer->getRefCount() == 1 );
    }

    co::exit();
    return EXIT_SUCCESS;
}
