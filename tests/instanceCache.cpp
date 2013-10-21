
/* Copyright (c) 2009-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef NDEBUG
#  define EQ_TEST_RUNTIME 600000 // 10min
#endif

#include <test.h>

#include <co/buffer.h>
#include <co/init.h>
#include <co/instanceCache.h>
#include <co/nodeCommand.h>
#include <co/localNode.h>
#include <co/objectDataICommand.h>
#include <co/objectDataOCommand.h>
#include <co/objectVersion.h>

#include <lunchbox/rng.h>
#include <lunchbox/thread.h>


// Tests the functionality of the instance cache

#define N_READER 1
#define RUNTIME 5000
#define COMMAND_SIZE 4096

lunchbox::Clock _clock;

class Reader : public lunchbox::Thread
{
public:
    Reader( co::InstanceCache& cache )
            : Thread(),
              _cache( cache )
        {}
    virtual ~Reader(){}

protected:
    virtual void run()
    {
        size_t hits = 0;
        size_t ops = 0;
        while( _clock.getTime64() < RUNTIME )
        {
            const lunchbox::UUID key( _rng.get< uint16_t >(), 0 );
            if( _cache[ key ] != co::InstanceCache::Data::NONE )
            {
                ++hits;
                _cache.release( key, 1 );
            }
            ++ops;
        }
        const uint64_t time = _clock.getTime64();
        std::cout << hits << " read hits in " << ops << " operations, "
                  << ops / time << " ops/ms" << std::endl;
    }

private:
    co::InstanceCache& _cache;
    lunchbox::RNG _rng;
};

int main( int argc, char **argv )
{
    co::init( argc, argv );
    co::ObjectDataOCommand out( co::Connections(), co::CMD_NODE_OBJECT_INSTANCE,
                                co::COMMANDTYPE_NODE, co::UUID(), 0,
                                co::uint128_t( 1 ), 0, 0, COMMAND_SIZE, true,
                                0 );
    co::LocalNodePtr node = new co::LocalNode;
    co::ObjectDataICommand in = out._getCommand( node );
    TESTINFO( in.isValid(), in );
    TEST( in.isLast( ));

    Reader** readers = static_cast< Reader** >(
        alloca( N_READER * sizeof( Reader* )));

    co::InstanceCache cache;
    lunchbox::RNG rng;

    size_t hits = 0;
    size_t ops = 0;

    for( lunchbox::UUID key; key.low() < 65536; ++key ) // Fill cache
        if( !cache.add( co::ObjectVersion( key, co::uint128_t( 1 )), 1, in ))
            break;

    _clock.reset();
    for( size_t i = 0; i < N_READER; ++i )
    {
        readers[ i ] = new Reader( cache );
        readers[ i ]->start();
    }

    while( _clock.getTime64() < RUNTIME )
    {
        const lunchbox::UUID id( 0, rng.get< uint16_t >( ));
        const co::ObjectVersion key( id, co::uint128_t( 1 ));
        if( cache[ key.identifier ] != co::InstanceCache::Data::NONE )
        {
            TEST( cache.release( key.identifier, 1 ));
            ++ops;
            if( cache.erase( key.identifier ))
            {
                TEST( cache.add( key, 1, in ));
                ops += 2;
                hits += 2;
            }
        }
        else if( cache.add( key, 1, in ))
            ++hits;
        ++ops;
    }

    const uint64_t time = _clock.getTime64();
    std::cout << hits << " write hits in " << ops << " operations, "
              << ops / time << " ops/ms" << std::endl;

    for( size_t i = 0; i < N_READER; ++i )
    {
        readers[ i ]->join();
        delete readers[ i ];
    }

    std::cout << cache << std::endl;

    for( lunchbox::UUID key; key.low() < 65536; ++key ) // Fill cache
    {
        if( cache[ key ] != co::InstanceCache::Data::NONE )
        {
            TEST( cache.release( key, 1 ));
            TEST( cache.erase( key ));
        }
    }

    for( lunchbox::UUID key; key.low() < 65536; ++key ) // Fill cache
    {
        TEST( cache[ key ] == co::InstanceCache::Data::NONE );
    }

    std::cout << cache << std::endl;

    TESTINFO( cache.getSize() == 0, cache.getSize( ));
    TEST( co::exit( ));
    return EXIT_SUCCESS;
}
