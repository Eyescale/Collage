
/* Copyright (c) 2012-2014, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2013-2014, Stefan.Eilemann@epfl.ch
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
/** @name Specialized input operators */
//@{
/** Read a std::string. */
template<> inline DataIStream& DataIStream::operator >> ( std::string& str )
{
    uint64_t nElems = 0;
    *this >> nElems;
    const uint64_t maxElems = getRemainingBufferSize();
    LBASSERTINFO( nElems <= maxElems,
                  nElems << " > " << maxElems );
    if( nElems == 0 )
        str.clear();
    else if( nElems <= maxElems )
        str.assign( static_cast< const char* >( getRemainingBuffer(nElems)),
                    size_t( nElems ));
    else
        str.assign(
            static_cast< const char* >( getRemainingBuffer(maxElems)),
                size_t( maxElems ));

    return *this;
}

/** Deserialize an object (id+version). */
template<> inline DataIStream& DataIStream::operator >> ( Object*& object )
{
    ObjectVersion data;
    *this >> data;
    LBASSERT( object->getID() == data.identifier );
    object->sync( data.version );
    return *this;
}

/** @cond IGNORE */
template< class T >
void DataIStream::_readArray( Array< T > array, const boost::true_type& )
{
    _read( array.data, array.getNumBytes( ));
    _swap( array );
}

/** Read an Array of non-POD data */
template< class T >
void DataIStream::_readArray( Array< T > array, const boost::false_type& )
{
    for( size_t i = 0; i < array.num; ++i )
        *this >> array.data[ i ];
}

template< class T > inline DataIStream&
DataIStream::operator >> ( lunchbox::RefPtr< T >& ptr )
{
    T* object = 0;
    *this >> object;
    ptr = object;
    return *this;
}

template< class T > inline DataIStream&
DataIStream::operator >> ( lunchbox::Buffer< T >& buffer )
{
    uint64_t nElems = 0;
    *this >> nElems;
    LBASSERTINFO( nElems < LB_BIT48,
                  "Out-of-sync co::DataIStream: " << nElems << " elements?" );
    buffer.resize( nElems );
    return *this >> Array< T >( buffer.getData(), nElems );
}


template< class T > inline DataIStream&
DataIStream::operator >> ( std::vector< T >& value )
{
    uint64_t nElems = 0;
    *this >> nElems;
    value.resize( nElems );
    for( uint64_t i = 0; i < nElems; ++i )
        *this >> value[i];
    return *this;
}

template< class K, class V > inline DataIStream&
DataIStream::operator >> ( std::map< K, V >& map )
{
    map.clear();
    uint64_t nElems = 0;
    *this >> nElems;
    for( uint64_t i = 0; i < nElems; ++i )
    {
        typename std::map< K, V >::key_type key;
        typename std::map< K, V >::mapped_type value;
        *this >> key >> value;
        map.insert( std::make_pair( key, value ));
    }
    return *this;
}

template< class T > inline DataIStream&
DataIStream::operator >> ( std::set< T >& value )
{
    value.clear();
    uint64_t nElems = 0;
    *this >> nElems;
    for( uint64_t i = 0; i < nElems; ++i )
    {
        T item;
        *this >> item;
        value.insert( item );
    }
    return *this;
}

template< class K, class V > inline DataIStream&
DataIStream::operator >> ( stde::hash_map< K, V >& map )
{
    map.clear();
    uint64_t nElems = 0;
    *this >> nElems;
    for( uint64_t i = 0; i < nElems; ++i )
    {
        typename stde::hash_map< K, V >::key_type key;
        typename stde::hash_map< K, V >::mapped_type value;
        *this >> key >> value;
        map.insert( std::make_pair( key, value ));
    }
    return *this;
}

template< class T > inline DataIStream&
DataIStream::operator >> ( stde::hash_set< T >& value )
{
    value.clear();
    uint64_t nElems = 0;
    *this >> nElems;
    for( uint64_t i = 0; i < nElems; ++i )
    {
        T item;
        *this >> item;
        value.insert( item );
    }
    return *this;
}

namespace
{
class ObjectFinder
{
public:
    ObjectFinder( const uint128_t& id ) : _id( id ) {}
    bool operator()( co::Object* candidate )
        { return candidate->getID() == _id; }

private:
    const uint128_t _id;
};
}

template<> inline void DataIStream::_swap( Array< void > ) const { /*NOP*/ }

template< typename O, typename C > inline void
DataIStream::deserializeChildren( O* object, const std::vector< C* >& old_,
                                  std::vector< C* >& result )
{
    ObjectVersions versions;
    *this >> versions;
    std::vector< C* > old = old_;

    // rebuild vector from serialized list
    result.clear();
    for( ObjectVersions::const_iterator i = versions.begin();
         i != versions.end(); ++i )
    {
        const ObjectVersion& version = *i;

        if( version.identifier == uint128_t( ))
        {
            result.push_back( 0 );
            continue;
        }

        typename std::vector< C* >::iterator j =
            lunchbox::find_if( old, ObjectFinder( version.identifier ));

        if( j == old.end( )) // previously unknown child
        {
            C* child = 0;
            object->create( &child );
            LocalNodePtr localNode = object->getLocalNode();
            LBASSERT( child );
            LBASSERT( !object->isMaster( ));

            LBCHECK( localNode->mapObject( child, version ));
            result.push_back( child );
        }
        else
        {
            C* child = *j;
            old.erase( j );
            if( object->isMaster( ))
                child->sync( VERSION_HEAD );
            else
                child->sync( version.version );

            result.push_back( child );
        }
    }

    while( !old.empty( )) // removed children
    {
        C* child = old.back();
        old.pop_back();
        if( !child )
            continue;

        if( child->isAttached() && !child->isMaster( ))
        {
            LocalNodePtr localNode = object->getLocalNode();
            localNode->unmapObject( child );
        }
        object->release( child );
    }
}
/** @endcond */

/** Optimized specialization to read a std::vector of uint8_t. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< uint8_t >& value )
{ return _readFlatVector( value );}

/** Optimized specialization to read a std::vector of uint16_t. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< uint16_t >& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of int16_t. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< int16_t >& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of uint32_t. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< uint32_t >& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of int32_t. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< int32_t >& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of uint64_t. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< uint64_t>& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of int64_t. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< int64_t >& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of float. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< float >& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of double. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< double >& value )
{ return _readFlatVector( value ); }

/** Optimized specialization to read a std::vector of ObjectVersion. */
template<> inline DataIStream&
DataIStream::operator >> ( std::vector< ObjectVersion >& value )
{ return _readFlatVector( value ); }
//@}
}
