
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "bufferCache.h"

#include "buffer.h"
#include "command.h"
#include "node.h"
#include <lunchbox/atomic.h>

#define COMPACT
//#define PROFILE
// 31300 hits, 35 misses, 297640 lookups, 126976b allocated in 31 packets
// 31300 hits, 35 misses, 49228 lookups, 135168b allocated in 34 packets

namespace co
{
namespace
{
enum Cache
{
    CACHE_SMALL,
    CACHE_BIG,
    CACHE_ALL
};

typedef std::vector< Buffer* > Data;
typedef Data::const_iterator DataCIter;

// minimum number of free packets, at least 2
static const int32_t _minFree[ CACHE_ALL ] = { 200, 20 };
static const uint32_t _freeShift = 1; // 'size >> shift' packets can be free

#ifdef PROFILE
static lunchbox::a_int32_t _hits;
static lunchbox::a_int32_t _misses;
static lunchbox::a_int32_t _lookups;
static lunchbox::a_int32_t _allocs;
static lunchbox::a_int32_t _frees;
#endif
}
namespace detail
{
class BufferCache
{
public:
    BufferCache()
        {
            for( size_t i = 0; i < CACHE_ALL; ++i )
            {
                cache[i].push_back( new co::Buffer( free[i] ));
                position[i] = cache[i].begin();
                free[i] = 1;
                maxFree[i] = _minFree[i];
            }
        }

    ~BufferCache()
        {
            for( size_t i = 0; i < CACHE_ALL; ++i )
            {
                LBASSERT( cache[i].size() == 1 );
                LBASSERT( cache[i].front()->isFree( ));

                delete cache[i].front();
                cache[i].clear();
            }
        }

    void flush()
        {
            for( size_t i = 0; i < CACHE_ALL; ++i )
            {
                Data& cache_ = cache[i];
                for( DataCIter j = cache_.begin(); j != cache_.end(); ++j )
                    (*j)->free();

                for( DataCIter j = cache_.begin(); j != cache_.end(); ++j )
                {
                    co::Buffer* buffer = *j;
                    LBASSERTINFO( buffer->isFree(),
                                  buffer << " rc " << buffer->getRefCount( ));
                    delete buffer;
                }
                LBASSERTINFO( size_t( free[i] ) == cache_.size(),
                              size_t( free[i] ) << " != " << cache_.size() );

                cache_.clear();
                cache_.push_back( new co::Buffer( free[i] ));
                free[i] = 1;
                maxFree[i] = _minFree[i];
                position[i] = cache_.begin();
            }
        }

    BufferPtr newBuffer( const Cache which )
        {
            _compact( CACHE_SMALL );
            _compact( CACHE_BIG );

            Data& cache_ = cache[ which ];
            const uint32_t cacheSize = uint32_t( cache_.size( ));
            lunchbox::a_int32_t& freeCounter = free[ which ];
            LBASSERTINFO( size_t( freeCounter ) <= cacheSize,
                          size_t( freeCounter ) << " > " << cacheSize );

            if( freeCounter > 0 )
            {
                LBASSERT( cacheSize > 0 );

                const DataCIter end = position[ which ];
                DataCIter& i = position[ which ];

                for( ++i; i != end; ++i )
                {
                    if( i == cache_.end( ))
                        i = cache_.begin();
#ifdef PROFILE
                    ++_lookups;
#endif
                    co::Buffer* buffer = *i;
                    if( buffer->isFree( ))
                    {
#ifdef PROFILE
                        const long hits = ++_hits;
                        if( (hits%1000) == 0 )
                        {
                            for( size_t j = 0; j < CACHE_ALL; ++j )
                            {
                                size_t size = 0;
                                const Data& cmds = cache_[ j ];
                                for( DataCIter k = cmds.begin();
                                     k != cmds.end(); ++k )
                                {
                                    size += (*k)->getMaxSize();
                                }
                                LBINFO << _hits << "/" << _hits + _misses
                                       << " hits, " << _lookups << " lookups, "
                                       << _free[j] << " of " << cmds.size()
                                       << " packets free (min " << _minFree[ j ]
                                       << " max " << _maxFree[ j ] << "), "
                                       << _allocs << " allocs, " << _frees
                                       << " frees, " << size / 1024 << "KB"
                                       << std::endl;
                            }
                        }
#endif
                        return buffer;
                    }
                }
            }
#ifdef PROFILE
            ++_misses;
#endif

            const uint32_t add = (cacheSize >> 3) + 1;
            for( size_t j = 0; j < add; ++j )
                cache_.push_back( new co::Buffer( freeCounter ));

            freeCounter += add;
            const int32_t num = int32_t( cache_.size() >> _freeShift );
            maxFree[ which ] = LB_MAX( _minFree[ which ], num );
            position[ which ] = cache_.begin();

#ifdef PROFILE
            _allocs += add;
#endif

            return cache_.back();
        }

    /** The caches. */
    Data cache[ CACHE_ALL ];

    /** Last lookup position in each cache. */
    DataCIter position[ CACHE_ALL ];

    /** The current number of free items in each cache */
    lunchbox::a_int32_t free[ CACHE_ALL ];

    /** The maximum number of free items in each cache */
    int32_t maxFree[ CACHE_ALL ];

private:
    void _compact( const Cache which )
        {
#ifdef COMPACT
            lunchbox::a_int32_t& currentFree = free[ which ];
            int32_t& maxFree_ = maxFree[ which ];
            if( currentFree <= maxFree_ )
                return;

            const int32_t target = maxFree_ >> 1;
            LBASSERT( target > 0 );
            Data& cache_ = cache[ which ];
            for( Data::iterator i = cache_.begin(); i != cache_.end(); )
            {
                const co::Buffer* cmd = *i;
                if( cmd->isFree( ))
                {
                    LBASSERT( currentFree > 0 );
                    i = cache_.erase( i );
                    delete cmd;

#  ifdef PROFILE
                    ++_frees;
#  endif
                    if( --currentFree <= target )
                        break;
                }
                else
                    ++i;
            }

            const int32_t num = int32_t( cache_.size() >> _freeShift );
            maxFree_ = LB_MAX( _minFree[ which ] , num );
            position[ which ] = cache_.begin();
#endif // COMPACT
        }
};
}

BufferCache::BufferCache()
        : _impl( new detail::BufferCache )
{}

BufferCache::~BufferCache()
{
    flush();
    delete _impl;
}

void BufferCache::flush()
{
    _impl->flush();
}

BufferPtr BufferCache::alloc( NodePtr node, LocalNodePtr localNode,
                              const uint64_t size )
{
    LB_TS_THREAD( _thread );
    LBASSERTINFO( size < LB_BIT48,
                  "Out-of-sync network stream: packet size " << size << "?" );

    const Cache which = (size > Buffer::getMinSize()) ? CACHE_BIG : CACHE_SMALL;
    BufferPtr buffer = _impl->newBuffer( which );
    buffer->alloc( node, localNode, size );
    return buffer;
}

std::ostream& operator << ( std::ostream& os, const BufferCache& cache )
{
    const Data& buffers = cache._impl->cache[ CACHE_SMALL ];
    os << lunchbox::disableFlush << "Cache has "
       << buffers.size() - cache._impl->free[ CACHE_SMALL ]
       << " used small packets:" << std::endl
       << lunchbox::indent << lunchbox::disableHeader;

    for( DataCIter i = buffers.begin(); i != buffers.end(); ++i )
    {
        Buffer* buffer = *i;
        if( !buffer->isFree( ))
            os << Command( buffer ) << std::endl;
    }
    return os << lunchbox::enableHeader << lunchbox::exdent
              << lunchbox::enableFlush;
}

}
