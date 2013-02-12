
/* Copyright (c) 2009-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_OBJECTVERSION_H
#define CO_OBJECTVERSION_H

#include <co/api.h>
#include <co/types.h>
#include <lunchbox/stdExt.h>
#include <iostream>

namespace co
{
    /** Special object version values */
    static const uint128_t VERSION_NONE( 0, 0 );
    static const uint128_t VERSION_FIRST( 0, 1 );
    static const uint128_t VERSION_OLDEST( 0, 0xfffffffffffffffcull );
    static const uint128_t VERSION_NEXT( 0, 0xfffffffffffffffdull );
    static const uint128_t VERSION_INVALID( 0, 0xfffffffffffffffeull );
    static const uint128_t VERSION_HEAD( 0, 0xffffffffffffffffull );

    /**
     * A helper struct bundling an object identifier and version.
     *
     * Primarily used for serialization. The struct either contains the object's
     * identifier and version (if it is registered or mapped), 0 and
     * VERSION_NONE if it is unmapped or if no object was given.
     */
    struct ObjectVersion
    {
        /** Construct a new, zero-initialized object version. @version 1.0 */
        CO_API ObjectVersion();

        /** Construct a new object version. @version 1.0 */
        CO_API ObjectVersion( const UUID& identifier, const uint128_t& version);

        /** Construct a new object version. @version 1.0 */
        CO_API ObjectVersion( const Object* object );

        /** Construct a new object version. @version 1.0 */
        template< class R > ObjectVersion( lunchbox::RefPtr< R > object )
            { *this = object.get(); }

        /** Assign a new identifier and version. @version 1.0 */
        CO_API ObjectVersion& operator = ( const Object* object );

        /** @return true if both structs contain the same values. @version 1.0*/
        bool operator == ( const ObjectVersion& value ) const
            {
                return ( identifier == value.identifier &&
                         version == value.version );
            }

        /** @return true if both structs have different values. @version 1.0*/
        bool operator != ( const ObjectVersion& value ) const
            {
                return ( identifier != value.identifier ||
                         version != value.version );
            }

        bool operator < ( const ObjectVersion& rhs ) const //!< @internal
            {
                return identifier < rhs.identifier ||
                    ( identifier == rhs.identifier && version < rhs.version );
            }

        bool operator > ( const ObjectVersion& rhs ) const //!< @internal
            {
                return identifier > rhs.identifier ||
                    ( identifier == rhs.identifier && version > rhs.version );
            }

        uint128_t identifier; //!< the object identifier
        uint128_t version; //!< the object version
    };

    inline std::ostream& operator << (std::ostream& os, const ObjectVersion& ov)
        { return os << "id " << ov.identifier << " v" << ov.version; }
}

namespace lunchbox
{
template<> inline void byteswap( co::ObjectVersion& value ) //!< @internal
{
    lunchbox::byteswap( value.identifier );
    lunchbox::byteswap( value.version );
}
}

LB_STDEXT_NAMESPACE_OPEN
#ifdef LB_STDEXT_MSVC
    /** ObjectVersion hash function. */
    template<>
    inline size_t hash_compare< co::ObjectVersion >::operator()
        ( const co::ObjectVersion& key ) const
    {
        const size_t hashVersion = hash_value( key.version );
        const size_t hashID = hash_value( key.identifier );

        return hash_value( hashVersion ^ hashID );
    }
#else
    /** ObjectVersion hash function. */
    template<> struct hash< co::ObjectVersion >
    {
        template< typename P > size_t operator()( const P& key ) const
        {
            return hash< uint64_t >()( hash_value( key.version ) ^
                                       hash_value( key.identifier ));
        }
    };
#endif
LB_STDEXT_NAMESPACE_CLOSE

#endif // CO_OBJECT_H
