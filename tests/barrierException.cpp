
/* Copyright (c) 2011-2015 Cedric Stalder <cedric.stalder@gmail.com>>
 *                         Stefan Eilemann <eile@eyescale.ch>
 *                         Daniel Nachbaur <danielnachbaur@gmail.com>
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
#include <co/exception.h>
#include <co/global.h>
#include <co/init.h>
#include <co/node.h>

#include <iostream>
#define CO_TEST_RUNTIME 6000
#define NSLAVES  10

class BarrierThread : public lunchbox::Thread
{
public:
    BarrierThread( const uint32_t nOps )
        : port( 0 )
        , _nOps( nOps )
        , _nTimeouts( 0 )
        , _node( new co::LocalNode )
    {
        co::ConnectionDescriptionPtr description =
            new co::ConnectionDescription;
        description->type = co::CONNECTIONTYPE_TCPIP;
        _node->addConnectionDescription( description );
        TEST( _node->listen( ));
        port = description->port;
    }

    size_t getNumTimeouts() const { return _nTimeouts; }
    uint16_t port;

protected:
    const uint32_t _nOps;
    size_t _nTimeouts;
    co::LocalNodePtr _node;
};

class ServerThread : public BarrierThread
{
public:
    ServerThread( const uint32_t nNodes, const uint32_t nOps )
        : BarrierThread( nOps )
    {
        _barrier = new co::Barrier( _node, _node->getNodeID(), nNodes + 1 );
        TEST( _barrier->isAttached( ));
        TEST( _barrier->getHeight() == nNodes + 1 );
        TEST( _barrier->getVersion() == co::VERSION_FIRST );
    }

    ~ServerThread()
    {
        _node->releaseObject( _barrier );
        delete _barrier;

        TEST( _node->close( ));
        _node = 0;
    }

    co::ObjectVersion getBarrierID() const
    { return co::ObjectVersion( _barrier ); }

protected:
    void run() final
    {
        for( uint32_t i = 0; i < _nOps; i++ )
        {
            const uint32_t timeout = co::Global::getIAttribute(
                     co::Global::IATTR_TIMEOUT_DEFAULT );
            if( !_barrier->enter( timeout ))
                ++_nTimeouts;
        }
    }
private:
    co::Barrier* _barrier;
};

class NodeThread : public BarrierThread
{
public:
    NodeThread( const co::ObjectVersion& barrierID, const uint32_t nOps,
                const uint32_t timeToSleep, const uint16_t serverPort )
         : BarrierThread( nOps )
         , _timeToSleep( timeToSleep )
    {
        co::NodePtr server = new co::Node;
        co::ConnectionDescriptionPtr serverDesc =
            new co::ConnectionDescription;
        serverDesc->port = serverPort;
        server->addConnectionDescription( serverDesc );
        TEST( _node->connect( server ));

        _barrier = new co::Barrier( _node, barrierID );
        TEST( _barrier->isGood( ));
        TEST( _barrier->isAttached( ));
        TESTINFO( _barrier->getVersion() == co::VERSION_FIRST,
                  _barrier->getVersion() );
        TEST( _barrier->getHeight() > 0 );
    }

    ~NodeThread()
    {
        _node->unmapObject( _barrier );
        delete _barrier;
        _barrier = 0;

        _node->flushCommands();
        TEST( _node->close());
        _node = 0;
    }

protected:
    void run() final
    {
        for( uint32_t i = 0; i < _nOps; ++i )
        {
            lunchbox::sleep( _timeToSleep );

            const uint32_t timeout = co::Global::getIAttribute(
                     co::Global::IATTR_TIMEOUT_DEFAULT );
            if( !_barrier->enter( timeout ))
                ++_nTimeouts;
        }
    }

private:
    const uint32_t _timeToSleep;
    co::Barrier* _barrier;
};

typedef std::vector< NodeThread* > NodeThreads;


/* the test perform no timeout */
void testNormal()
{
    co::Global::setIAttribute( co::Global::IATTR_TIMEOUT_DEFAULT, 10000 );
    NodeThreads nodeThreads;
    nodeThreads.resize(NSLAVES);

    ServerThread server( NSLAVES, 1 );
    server.start();

    for( uint32_t i = 0; i < NSLAVES; i++ )
    {
        nodeThreads[i] = new NodeThread( server.getBarrierID(), 1, 0,
                                         server.port );
        nodeThreads[i]->start();
    }

    server.join();

    for( uint32_t i = 0; i < NSLAVES; i++ )
    {
        nodeThreads[i]->join();
        TEST( nodeThreads[i]->getNumTimeouts() == 0 );
        delete nodeThreads[i];
    }

    TEST( server.getNumTimeouts() == 0 );
}

/* the test perform no timeout */
void testException()
{
    co::Global::setIAttribute( co::Global::IATTR_TIMEOUT_DEFAULT, 2000 );
    NodeThreads nodeThreads;
    nodeThreads.resize( NSLAVES );

    ServerThread server( NSLAVES, 1 );
    server.start();

    for( uint32_t i = 0; i < NSLAVES - 1; i++ )
    {
        nodeThreads[i] = new NodeThread( server.getBarrierID(), 1, 0,
                                         server.port );
        nodeThreads[i]->start();
    }

    TEST( server.join() );
    TEST( server.getNumTimeouts() == 1 );
    for( uint32_t i = 0; i < NSLAVES - 1; i++ )
    {
        TEST( nodeThreads[i]->join() );
        TEST( nodeThreads[i]->getNumTimeouts() == 1 );
        delete nodeThreads[i];
    }
}

void testSleep()
{
    co::Global::setIAttribute( co::Global::IATTR_TIMEOUT_DEFAULT, 200 );
    static const size_t numThreads = 5;
    NodeThreads nodeThreads( numThreads );
    ServerThread server( numThreads, 1 );
    server.start();

    uint32_t sleep = 50;
    for( size_t i = 0; i < numThreads; ++i )
    {
        sleep += 50;
        nodeThreads[i] = new NodeThread( server.getBarrierID(), 5, sleep,
                                         server.port );
        nodeThreads[i]->start();
    }

    TEST( server.join() );

    for( size_t i = 0; i < numThreads; ++i )
    {
        TEST( nodeThreads[i]->join( ));
    }

    // Bug: if the first nodeThread is deleted before the the second nodeThread,
    // the barrier send unblock will fail because it is trying to send to an
    // unexisting connection node. Can this happen in Collage ?
    for( size_t i = 0; i < numThreads; ++i )
        delete nodeThreads[i];
}

int main( int argc, char **argv )
{
    TEST( co::init( argc, argv ));

    testNormal();
    testException();
    testSleep();

    co::exit();
    return EXIT_SUCCESS;
}
