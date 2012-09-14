
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include <co/object.h>
#include <co/objectVersion.h>

namespace co
{
    /** @name Specialized output operators */
    //@{
    /** Write a std::string. */
    template<>
    inline DataOStream& DataOStream::operator << ( const std::string& str )
    {
        const uint64_t nElems = str.length();
        _write( &nElems, sizeof( nElems ));
        if ( nElems > 0 )
            _write( str.c_str(), nElems );

        return *this;
    }

    /** Write an object identifier and version. */
    template<> inline DataOStream&
    DataOStream::operator << ( const Object* const& object )
    {
        LBASSERT( !object || object->isAttached( ));
        (*this) << ObjectVersion( object );
        return *this;
    }

/** @cond IGNORE */
    template< class T > inline DataOStream&
    DataOStream::operator << ( const lunchbox::Buffer< T >& buffer )
    {
        return (*this) << buffer.getSize()
                       << Array< const T >( buffer.getData(), buffer.getSize());
    }

    template< class T > inline DataOStream&
    DataOStream::operator << ( const std::vector< T >& value )
    {
        const uint64_t nElems = value.size();
        *this << nElems;
        for( uint64_t i = 0; i < nElems; ++i )
            *this << value[i];
        return *this;
    }

    template< class K, class V > inline DataOStream&
    DataOStream::operator << ( const std::map< K, V >& value )
    {
        const uint64_t nElems = value.size();
        *this << nElems;
        for( typename std::map< K, V >::const_iterator it = value.begin();
             it != value.end(); ++it )
        {
            *this << it->first << it->second;
        }
        return *this;
    }

    template< class T > inline DataOStream&
    DataOStream::operator << ( const std::set< T >& value )
    {
        const uint64_t nElems = value.size();
        *this << nElems;
        for( typename std::set< T >::const_iterator it = value.begin();
             it != value.end(); ++it )
        {
            *this << *it;
        }
        return *this;
    }

    template< class K, class V > inline DataOStream&
    DataOStream::operator << ( const stde::hash_map< K, V >& value )
    {
        const uint64_t nElems = value.size();
        *this << nElems;
        for( typename stde::hash_map< K, V >::const_iterator it = value.begin();
             it != value.end(); ++it )
        {
            *this << it->first << it->second;
        }
        return *this;
    }

    template< class T > inline DataOStream&
    DataOStream::operator << ( const stde::hash_set< T >& value )
    {
        const uint64_t nElems = value.size();
        *this << nElems;
        for( typename stde::hash_set< T >::const_iterator it = value.begin();
             it != value.end(); ++it )
        {
            *this << *it;
        }
        return *this;
    }

    template< typename C > inline void
    DataOStream::serializeChildren( const std::vector<C*>& children )
    {
        const uint64_t nElems = children.size();
        (*this) << nElems;

        for( typename std::vector< C* >::const_iterator i = children.begin();
             i != children.end(); ++i )
        {
            C* child = *i;
            (*this) << ObjectVersion( child );
            LBASSERTINFO( !child || child->isAttached(),
                          "Found unmapped object during serialization" );
        }
    }
/** @endcond */

    /** Optimized specialization to write a std::vector of uint8_t. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< uint8_t >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of uint16_t. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< uint16_t >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of int16_t. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< int16_t >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of uint32_t. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< uint32_t >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of int32_t. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< int32_t >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of uint64_t. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< uint64_t >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of int64_t. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< int64_t >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of float. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< float >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of double. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< double >& value )
    { return _writeFlatVector( value ); }

    /** Optimized specialization to write a std::vector of ObjectVersion. */
    template<> inline DataOStream&
    DataOStream::operator << ( const std::vector< ObjectVersion >& value )
    { return _writeFlatVector( value ); }
    //@}
}
