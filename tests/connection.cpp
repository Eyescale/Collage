
/* Copyright (c) 2010-2015, Stefan Eilemann <eile@equalizergraphics.com>
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

#define PACKETSIZE (123456)
#define RUNTIME (1000) // ms

lunchbox::Monitor< bool > s_done( false );

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
    explicit Reader( co::ConnectionPtr connection ) : connection_( connection )
        { TEST( start( )); }

    void run() override
    {
        co::ConnectionPtr listener = connection_;
        connection_ = listener->acceptSync();
        TESTINFO( connection_, listener->getDescription( ));

        co::Buffer buffer;
        co::BufferPtr syncBuffer;
        buffer.reserve( PACKETSIZE );
        uint64_t& sequence = *reinterpret_cast< uint64_t* >( buffer.getData( ));
        sequence = 0;
        uint64_t i = 0;

        s_done = false;

        while( sequence != 0xdeadbeef )
        {
            connection_->recvNB( &buffer, PACKETSIZE );
            TEST( connection_->recvSync( syncBuffer ));
            TEST( syncBuffer == &buffer );
            TEST( buffer.getSize() == PACKETSIZE );
            TEST( sequence == ++i || sequence == 0xdeadbeef );
            buffer.setSize( 0 );
        }

        s_done = true;
        connection_->recvNB( &buffer, PACKETSIZE );
        TEST( !connection_->recvSync( syncBuffer ));
#ifndef _WIN32
        TEST( connection_->isClosed( ));
#endif
        connection_ = 0;
    }

private:
    co::ConnectionPtr connection_;
};

class Latency : public lunchbox::Thread
{
public:
    explicit Latency( co::ConnectionPtr connection ) : connection_( connection )
        { TEST( start( )); }

    void run() override
    {
        connection_ = connection_->acceptSync();
        co::Buffer buffer;
        co::BufferPtr syncBuffer;
        buffer.reserve( sizeof( uint64_t ));
        uint64_t& sequence = *reinterpret_cast< uint64_t* >( buffer.getData( ));
        sequence = 0;
        s_done = false;

        while( sequence != 0xC0FFEE )
        {
            connection_->recvNB( &buffer, sizeof( uint64_t ));
            TEST( connection_->recvSync( syncBuffer ));
            buffer.setSize( 0 );
        }

        s_done = true;
        connection_->recvNB( &buffer, sizeof( uint64_t ));
        TEST( !connection_->recvSync( syncBuffer ));
#ifndef _WIN32
        TEST( connection_->isClosed( ));
#endif
        connection_ = 0;
    }

private:
    co::ConnectionPtr connection_;
};

bool _initialize( co::ConnectionDescriptionPtr desc,
                  co::ConnectionPtr& listener,
                  co::ConnectionPtr& writer )
{
    if( desc->type >= co::CONNECTIONTYPE_MULTICAST )
        desc->setHostname( "239.255.12.34" );
    else
        desc->setHostname( "127.0.0.1" );

    listener = co::Connection::create( desc );
    if( !listener )
    {
        std::cout << desc->type << ": not supported" << std::endl;
        return false;
    }

    switch( desc->type ) // different connections, different semantics...
    {
    case co::CONNECTIONTYPE_PIPE:
        writer = listener;
        break;

    case co::CONNECTIONTYPE_RSP:
        desc->port = co::ConnectionDescription::RANDOM_MULTICAST_PORT;
        TESTINFO( listener->listen(), desc );
        listener->acceptNB();

        writer = listener;
        break;

    default:
    {
        const bool listening = listener->listen();
        if( !listening && desc->type == co::CONNECTIONTYPE_RDMA )
            return false; // No local IB adapter up

        TESTINFO( listening, desc );
        listener->acceptNB();

        writer = co::Connection::create( desc );

        break;
    }
    }

    TEST( writer );
    return true;
}
}

int main( int argc, char **argv )
{
    TEST(( PACKETSIZE % 8 ) == 0 );
    co::init( argc, argv );

    for( size_t i = 0; types[i] != co::CONNECTIONTYPE_NONE; ++i )
    {
        co::ConnectionDescriptionPtr desc = new co::ConnectionDescription;
        desc->type = types[i];

        co::ConnectionPtr writer;
        co::ConnectionPtr listener;

        if( !_initialize( desc, listener, writer ))
            continue;

        Reader readThread( listener );
        if( desc->type != co::CONNECTIONTYPE_RSP )
            TEST( writer->connect( ));

        uint64_t out[ PACKETSIZE / 8 ];

        lunchbox::Clock clock;
        uint64_t sequence = 0;

        while( clock.getTime64() < RUNTIME )
        {
            out[0] = ++sequence;
            TEST( writer->send( out, PACKETSIZE ));
        }

        out[0] = 0xdeadbeef;
        TEST( writer->send( out, PACKETSIZE ));

        s_done.waitEQ( true );
        writer->close();
        readThread.join();
        listener->close();
        const float bwTime = clock.getTimef();
        const uint64_t numBW = sequence;

        TEST( _initialize( desc, listener, writer ));
        Latency latency( listener );
        if( desc->type != co::CONNECTIONTYPE_RSP )
            TEST( writer->connect( ));
        sequence = 0;
        clock.reset();

        while( clock.getTime64() < RUNTIME )
        {
            ++sequence;
            TEST( writer->send( &sequence, sizeof( uint64_t )));
        }

        sequence = 0xC0FFEE;
        TEST( writer->send( &sequence, sizeof( uint64_t )));

        s_done.waitEQ( true );
        writer->close();
        latency.join();
        listener->close();

        const float latencyTime = clock.getTimef();
        const float mFactor = 1024.f / 1024.f * 1000.f;

        std::cout << desc->type << ": "
                  << (numBW+1) * PACKETSIZE / mFactor / bwTime
                  << " MBps, " << (sequence+1) / mFactor / latencyTime
                  << " Mpps" << std::endl;

        if( listener == writer )
            listener = 0;

        TESTINFO( !listener || listener->getRefCount() == 1,
                  listener->getRefCount());
        TEST( writer->getRefCount() == 1 );
    }

    co::exit();
    return EXIT_SUCCESS;
}
