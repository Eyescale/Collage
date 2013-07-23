
/* Copyright (c) 2006-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *
 * This file is part of Collage <https://github.com/Eyescale/Collage>
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
#include "bufferListener.h"
#include "iCommand.h"
#include "node.h"

#include <lunchbox/atomic.h>

//#define PROFILE
// 31300 hits, 35 misses, 297640 lookups, 126976b allocated in 31 buffers
// 31300 hits, 35 misses, 49228 lookups, 135168b allocated in 34 buffers
//
// The buffer cache periodically frees allocated buffers to bound memory usage:
// * 'minFree' buffers (given in ctor) are always kept free
// * above 'size >> _maxFreeShift' free buffers compaction occurs
// * compaction tries to reach '(size >> _maxFreeShift) >> _targetShift' buffers
//
// In other words, using the values below, if more than half of the buffers are
// free, the cache is compacted to until one quarter of the buffers is free.

namespace co
{
namespace
{
typedef std::vector< Buffer* > Data;
typedef Data::const_iterator DataCIter;

static const uint32_t _maxFreeShift = 1; // _maxFree = size >> shift
static const uint32_t _targetShift = 1; // _targetFree = _maxFree >> shift

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
class BufferCache : public BufferListener
{
public:
    BufferCache( const int32_t minFree )
        : _minFree( minFree )
    {
        LBASSERT( minFree > 1);
        flush();
    }

    ~BufferCache()
    {
        LBASSERT( _cache.size() == 1 );
        LBASSERT( _cache.front()->isFree( ));

        delete _cache.front();
        _cache.clear();
    }

    void flush()
    {
        for( DataCIter i = _cache.begin(); i != _cache.end(); ++i )
        {
            co::Buffer* buffer = *i;
            //LBASSERTINFO( buffer->isFree(), *buffer );
            delete buffer;
        }
        LBASSERTINFO( size_t( _free ) == _cache.size(),
                      size_t( _free ) << " != " << _cache.size() );

        _cache.clear();
        _cache.push_back( new co::Buffer( this ));
        _free = 1;
        _maxFree = _minFree;
        _position = _cache.begin();
    }

    BufferPtr newBuffer()
    {
        const uint32_t cacheSize = uint32_t( _cache.size( ));
        LBASSERTINFO( size_t( _free ) <= cacheSize,
                      size_t( _free ) << " > " << cacheSize );

        if( _free > 0 )
        {
            LBASSERT( cacheSize > 0 );

            const DataCIter end = _position;
            DataCIter& i = _position;

            for( ++i; i != end; ++i )
            {
                if( i == _cache.end( ))
                    i = _cache.begin();
#ifdef PROFILE
                ++_lookups;
#endif
                co::Buffer* buffer = *i;
                if( !buffer->isFree( ))
                    continue;

#ifdef PROFILE
                const long hits = ++_hits;
                if( (hits%1000) == 0 )
                {
                    size_t size = 0;
                    for( DataCIter j = _cache.begin(); j != _cache.end(); ++j )
                            size += (*j)->getMaxSize();

                    LBINFO << _hits << "/" << _hits + _misses << " hits, "
                           << _lookups << " lookups, " << _free << " of "
                           << _cache.size() << " buffers free (min " << _minFree
                           << " max " << _maxFree << "), " << _allocs
                           << " allocs, " << _frees << " frees, "
                           << size / 1024 << "KB" << std::endl;
                }
#endif
                --_free;
                return buffer;
            }
        }

        const uint32_t add = (cacheSize >> 3) + 1;
        for( size_t j = 0; j < add; ++j )
            _cache.push_back( new co::Buffer( this ));

        _free += add - 1;
        const int32_t num = int32_t( _cache.size() >> _maxFreeShift );
        _maxFree = LB_MAX( _minFree, num );
        _position = _cache.begin();

#ifdef PROFILE
        ++_misses;
        _allocs += add;
#endif
        return _cache.back();
    }

    void compact()
    {
        if( _free <= _maxFree )
            return;

        const int32_t tgt = _maxFree >> _targetShift;
        const int32_t target = LB_MAX( tgt, _minFree );
        LBASSERT( target > 0 );
        for( Data::iterator i = _cache.begin(); i != _cache.end(); )
        {
            const co::Buffer* cmd = *i;
            if( cmd->isFree( ))
            {
                LBASSERT( _free > 0 );
#  ifdef PROFILE
                ++_frees;
#  endif
                i = _cache.erase( i );
                delete cmd;

                if( --_free <= target )
                    break;
            }
            else
                ++i;
        }

        const int32_t num = int32_t( _cache.size() >> _maxFreeShift );
        _maxFree = LB_MAX( _minFree, num );
        _position = _cache.begin();
    }

private:
    friend std::ostream& co::operator << (std::ostream&,const co::BufferCache&);

    Data _cache;
    DataCIter _position; //!< Last lookup position
    lunchbox::a_int32_t _free; //!< The current number of free items

    const int32_t _minFree;
    int32_t _maxFree; //!< The maximum number of free items

    virtual void notifyFree( co::Buffer* )
    {
        ++_free;
    }
};
}

BufferCache::BufferCache( const int32_t minFree )
        : _impl( new detail::BufferCache( minFree ))
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

BufferPtr BufferCache::alloc( const uint64_t size )
{
    LB_TS_SCOPED( _thread );
    LBASSERTINFO( size >= COMMAND_ALLOCSIZE, size );
    LBASSERTINFO( size < LB_BIT48,
                  "Out-of-sync network stream: buffer size " << size << "?" );

    BufferPtr buffer = _impl->newBuffer();
    LBASSERT( buffer->getRefCount() == 1 );

    buffer->reserve( size );
    buffer->resize( 0 );
    return buffer;
}

void BufferCache::compact()
{
    _impl->compact();
}

std::ostream& operator << ( std::ostream& os, const BufferCache& cache )
{
    const Data& buffers = cache._impl->_cache;
    os << lunchbox::disableFlush << "Cache has "
       << buffers.size() - cache._impl->_free << " used buffers:" << std::endl
       << lunchbox::indent << lunchbox::disableHeader;

    for( DataCIter i = buffers.begin(); i != buffers.end(); ++i )
    {
        Buffer* buffer = *i;
        if( !buffer->isFree( ))
            os << ICommand( 0, 0, buffer, false /*swap*/ ) << std::endl;
    }
    return os << lunchbox::enableHeader << lunchbox::exdent
              << lunchbox::enableFlush;
}

}
