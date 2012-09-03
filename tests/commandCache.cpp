
/* Copyright (c) 2011-2012, Stefan Eilemann <eile@eyescale.ch>
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

#include <co/buffer.h>
#include <co/bufferCache.h>
#include <co/command.h>
#include <co/commandQueue.h>
#include <co/dispatcher.h>
#include <co/init.h>
#include <co/localNode.h>
#include <co/nodeOCommand.h>
#include <lunchbox/clock.h>

// Tests the functionality of the network packet cache

#define N_READER 13
#define RUNTIME 5000

uint64_t rTime = 1;
lunchbox::SpinLock _lock;

class Reader : public co::Dispatcher, public lunchbox::Thread
{
public:
    bool _cmd( co::Command& command ) { return true; }
    bool _cmdStop( co::Command& command ) { _running = false; return true; }

    Reader() : index( 0 ), _running( false )
        {
            registerCommand( 0u,
                             co::CommandFunc< Reader >( this, &Reader::_cmd ),
                             &_queue );
            registerCommand( 1u,
                             co::CommandFunc<Reader>( this, &Reader::_cmdStop ),
                             &_queue );
        }
    virtual ~Reader(){}

    size_t index;

protected:
    virtual void run()
        {
            lunchbox::Clock clock;
            _running = true;
            while( _running )
            {
                co::BufferPtr buffer = _queue.pop();
                TEST( buffer->getRefCount() > 0 );
                // Writer callstack + self reference
                TESTINFO( index == 0 || buffer->getRefCount() < 7,
                          index << ", " << buffer->getRefCount() );
                {
                    co::Command command( buffer );
                    TEST( command( ));
                }
                TEST( buffer->getRefCount() > 0 );
            }
            TEST( _queue.isEmpty( ));
            lunchbox::ScopedFastWrite mutex( _lock );
            rTime = clock.getTime64();
        }

private:
    co::CommandQueue _queue;
    bool _running;
};

int main( int argc, char **argv )
{
    co::init( argc, argv );
    {
        Reader readers[ N_READER ];
        for( size_t i = 0; i < N_READER; ++i )
        {
            readers[i].index = i;
            readers[i].start();
        }

        co::BufferCache cache;
        co::LocalNodePtr node = new co::LocalNode;
        size_t nOps = 0;

        lunchbox::Clock clock;
        while( clock.getTime64() < RUNTIME )
        {
            co::BufferPtr buffer = cache.alloc( node, node,
                                                co::NodeOCommand::getSize( ));
            co::Command command( buffer );
            command.setCommand( 0 );

            readers[0].dispatchCommand( command );

            for( size_t i = 1; i < N_READER; ++i )
            {
#if 1
                co::BufferPtr clone = cache.clone( buffer );
#else
                co::BufferPtr clone = cache.alloc( node, node,
                                                   co::NodeOCommand::getSize());
                co::Command cloneCommand( clone );
                cloneCommand.setCommand( 0 );
#endif
                co::Command clonedCmd( clone );
                readers[i].dispatchCommand( clonedCmd );
            }
            ++nOps;
        }
        const uint64_t wTime = clock.getTime64();

        for( size_t i = 0; i < N_READER; ++i )
        {
            co::BufferPtr buffer = cache.alloc( node, node,
                                                co::NodeOCommand::getSize( ));
            co::Command command( buffer );
            command.setCommand( 1 );

            readers[i].dispatchCommand( command );
            readers[i].join();
        }

        std::cout << N_READER * nOps / wTime << " write, "
                  << N_READER * nOps / rTime << " read ops/ms" << std::endl;
    }

    TEST( co::exit( ));
    return EXIT_SUCCESS;
}
