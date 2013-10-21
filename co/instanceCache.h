
/* Copyright (c) 2009-2012, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_INSTANCECACHE_H
#define CO_INSTANCECACHE_H

#include <co/api.h>
#include <co/types.h>

#include <lunchbox/clock.h>     // member
#include <lunchbox/lock.h>      // member
#include <lunchbox/lockable.h>  // member
#include <lunchbox/stdExt.h>    // member
#include <lunchbox/thread.h>    // member
#if LUNCHBOX_VERSION_GE( 1, 9, 0 )
#  include <lunchbox/uint128_t.h>      // member
#else
#  include <lunchbox/uuid.h>      // member
#endif

#include <iostream>

namespace co
{
    /** @internal A thread-safe cache for object instance data. */
    class InstanceCache
    {
    public:
        /** Construct a new instance cache. */
        CO_API InstanceCache( const uint64_t maxSize = LB_100MB );

        /** Destruct this instance cache. */
        CO_API ~InstanceCache();

        /**
         * Add a new command to the instance cache.
         *
         * @param rev the object identifier and version.
         * @param instanceID the master instance ID.
         * @param command The command to add.
         * @param usage pre-set usage count.
         * @return true if the command was entered, false if not.
         */
        CO_API bool add( const ObjectVersion& rev, const uint32_t instanceID,
                         ICommand& command, const uint32_t usage = 0 );

        /** Remove all items from the given node. */
        void remove( const NodeID& node );

        /** One cache entry */
        struct Data
        {
            Data();
            CO_API bool operator != ( const Data& rhs ) const;
            CO_API bool operator == ( const Data& rhs ) const;

            uint32_t masterInstanceID; //!< The instance ID of the master object
            ObjectDataIStreamDeque versions; //!< all cached data
            CO_API static const Data NONE; //!< '0' return value
        };

        /**
         * Direct access to the cached instance data for the given object id.
         *
         * The instance data for the given object has to be released by the
         * caller, unless 0 has been returned. Not all returned data stream
         * might be ready.
         *
         * @param id the identifier of the object to look up.
         * @return the list of cached instance datas, or Data::NONE if no data
         *         is cached for this object.
         */

        CO_API const Data& operator[]( const UUID& id );

        /**
         * Release the retrieved instance data of the given object.
         *
         * @param id the identifier of the object to release.
         * @param count the number of access operations to release
         * @return true if the element was unpinned, false if it is not in the
         *         instance cache.
         */
        CO_API bool release( const UUID& id, const uint32_t count );

        /**
         * Erase all the data for the given object.
         *
         * The data does not have to be accessed, i.e., release has been called
         * for each previous access.
         *
         * @return true if the element was erased, false otherwise.
         */
        CO_API bool erase( const UUID& id );

        /** @return the number of bytes used by the instance cache. */
        uint64_t getSize() const { return _size; }

        /** @return the maximum number of bytes used by the instance cache. */
        uint64_t getMaxSize() const { return _maxSize; }

        /** Remove all items which are older than the given time. */
        void expire( const int64_t age );

        bool isEmpty() { return _items->empty(); }

    private:
        struct Item
        {
            Item();
            Data data;
            unsigned used;
            unsigned access;
            NodeID from;

            typedef std::deque< int64_t > TimeDeque;
            TimeDeque times;
        };

        typedef stde::hash_map< lunchbox::uint128_t, Item > ItemHash;
        typedef ItemHash::iterator ItemHashIter;
        lunchbox::Lockable< ItemHash > _items;

        const uint64_t _maxSize; //!<high-water mark to start releasing commands
        uint64_t _size;          //!< Current number of bytes stored

        const lunchbox::Clock _clock;  //!< Clock for item expiration

        void _releaseItems( const uint32_t minUsage );
        void _releaseStreams( InstanceCache::Item& item );
        void _releaseStreams( InstanceCache::Item& item,
                              const int64_t minTime );
        void _releaseFirstStream( InstanceCache::Item& item );
        void _deleteStream( ObjectDataIStream* iStream );

        LB_TS_VAR( _thread );
    };

    CO_API std::ostream& operator << ( std::ostream&, const InstanceCache& );
}
#endif //CO_INSTANCECACHE_H
