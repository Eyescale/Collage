
/* Copyright (c) 2014, Daniel Nachbaur <danielnachbaur@gmail.com>
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
#include <lunchbox/mtQueue.h>
#include <lunchbox/spinLock.h>

lunchbox::Monitorb _registered( false );
lunchbox::Monitorb _mapped( false );
lunchbox::Monitorb _done( false );
co::ObjectVersion _barrierID;
co::Barrier* _barrier;
lunchbox::MTQueue< co::uint128_t > _versions;
const size_t _latency( 1 );
const size_t _numThreads( 3 );
const size_t _numIterations( 100 );

class ServerThread : public lunchbox::Thread
{
public:
    ServerThread( co::LocalNodePtr server )
        : _node( new co::LocalNode )
        , _server( server )
    {
        TEST( _node->listen( ));

        co::NodePtr node = new co::Node;
        co::ConnectionDescriptionPtr description =
            server->getConnectionDescriptions().front();
        node->addConnectionDescription( description );
        TEST( _node->connect( node ));
    }

    void run() override
    {
        setName( "MasterThread" );
        co::Barrier barrier( _node, _server->getNodeID(), _numThreads );
        barrier.setAutoObsolete( _latency + 1 );
        _barrierID = co::ObjectVersion( &barrier );
        _registered = true;
        _versions.setMaxSize( (_latency + 1) * _numThreads );

        _mapped.waitEQ( true );

        while( !_done )
        {
            TEST( barrier.isGood());

            const co::uint128_t& version = barrier.commit();
            for( size_t i = 0; i < _numThreads; ++i )
                _versions.push( version );
        }
    }
private:
    co::LocalNodePtr _node;
    co::LocalNodePtr _server;
};


class WorkerThread : public lunchbox::Thread
{
public:
    co::LocalNodePtr _node;
    const bool _master;
    WorkerThread( co::LocalNodePtr node, const bool master )
        : _node( node ), _master( master ){}

    void run() override
    {
        if( _master )
            setName( "MasterWorker" );
        else
            setName( "SlaveWorker" );

        _registered.waitEQ( true );
        if( _master )
        {
            _barrier = new co::Barrier( _node, _barrierID );
            _mapped = true;
        }
        else
            _mapped.waitEQ( true );

        for( size_t i = 0; i < _numIterations; ++i )
        {
            const co::uint128_t& version = _versions.pop();

            {
                static lunchbox::SpinLock lock;
                lunchbox::ScopedFastWrite mutex( lock );
                _barrier->sync( version );
            }

            TEST( _barrier->isGood());
            _barrier->enter();
        }

        if( _master )
        {
            _done = true;
            _versions.clear();
            _node->releaseObject( _barrier );
            delete _barrier;
        }
    }
};

int main( int argc, char **argv )
{
    TEST( co::init( argc, argv ));

    co::LocalNodePtr node = new co::LocalNode;
    co::ConnectionDescriptionPtr desc = new co::ConnectionDescription;
    node->addConnectionDescription( desc );
    TEST( node->listen( ));

    ServerThread master( node );
    master.start();

    std::vector< WorkerThread* > workers;
    for( size_t i = 0; i < _numThreads; ++i )
        workers.push_back( new WorkerThread( node, i == 0 ));

    for( size_t i = 0; i < _numThreads; ++i )
        workers[i]->start();

    for( size_t i = 0; i < _numThreads; ++i )
        workers[i]->join();

    master.join();
    node->close();

    TEST( co::exit( ));
    return EXIT_SUCCESS;
}
