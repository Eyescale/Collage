
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *               2009-2010, Cedric Stalder <cedric.stalder@gmail.com>
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

#ifndef CO_DATAISTREAM_H
#define CO_DATAISTREAM_H

#include <co/api.h>
#include <co/array.h> // used inline
#include <co/types.h>

#include <lunchbox/stdExt.h>

#include <map>
#include <set>
#include <vector>

namespace co
{
namespace detail { class DataIStream; }

/** A std::istream-like input data stream for binary data. */
class DataIStream
{
public:
    /** @name Internal */
    //@{
    /** @internal @return the number of remaining buffers. */
    virtual size_t nRemainingBuffers() const = 0;

    virtual uint128_t getVersion() const = 0; //!< @internal
    virtual void reset() { _reset(); } //!< @internal
    void setSwapping( const bool onOff ); //!< @internal enable endian swap
    CO_API bool isSwapping() const; //!< @internal
    DataIStream& operator = ( const DataIStream& rhs ); //!< @internal
    //@}

    /** @name Data input */
    //@{
    /** Read a plain data item. @version 1.0 */
    template< class T > DataIStream& operator >> ( T& value )
        { _read( &value, sizeof( value )); _swap( value ); return *this; }

    /** Read a C array. @version 1.0 */
    template< class T > DataIStream& operator >> ( Array< T > array );

    /** Read a lunchbox::Buffer. @version 1.0 */
    template< class T > DataIStream& operator >> ( lunchbox::Buffer< T >& );

    /** Read a std::vector of serializable items. @version 1.0 */
    template< class T > DataIStream& operator >> ( std::vector< T >& );

    /** Read a std::map of serializable items. @version 1.0 */
    template< class K, class V >
    DataIStream& operator >> ( std::map< K, V >& );

    /** Read a std::set of serializable items. @version 1.0 */
    template< class T > DataIStream& operator >> ( std::set< T >& );

    /** Read a stde::hash_map of serializable items. @version 1.0 */
    template< class K, class V >
    DataIStream& operator >> ( stde::hash_map< K, V >& );

    /** Read a stde::hash_set of serializable items. @version 1.0 */
    template< class T > DataIStream& operator >> ( stde::hash_set< T >& );

    /**
     * @define CO_IGNORE_BYTESWAP: If set, no byteswapping of transmitted data
     * is performed. Enable when you get unresolved symbols for
     * lunchbox::byteswap and you don't care about mixed-endian environments.
     */
#  ifdef CO_IGNORE_BYTESWAP
    /** Byte-swap a plain data item. @version 1.0 */
    template< class T > static void swap( T& ) { /* nop */ }
#  else
    /** Byte-swap a plain data item. @version 1.0 */
    template< class T > static void swap( T& v ) { lunchbox::byteswap(v); }
#  endif

    /** @internal
     * Deserialize child objects.
     *
     * Existing children are synced to the new version. New children are created
     * by calling the <code>void create( C** child )</code> method on the
     * object, and are registered or mapped to the object's session. Removed
     * children are released by calling the <code>void release( C* )</code>
     * method on the object. The resulting child vector is created in
     * result. The old and result vector can be the same object, the result
     * vector is cleared and rebuild completely.
     */
    template< typename O, typename C >
    void deserializeChildren( O* object, const std::vector< C* >& old,
                              std::vector< C* >& result );

    /** @deprecated
     * Get the pointer to the remaining data in the current buffer.
     *
     * The usage of this method is discouraged, no endian conversion or bounds
     * checking is performed by the DataIStream on the returned raw pointer.
     *
     * The buffer is advanced by the given size. If not enough data is present,
     * 0 is returned and the buffer is unchanged.
     *
     * The data written to the DataOStream by the sender is bucketized, it is
     * sent in multiple blocks. The remaining buffer and its size points into
     * one of the buffers, i.e., not all the data sent is returned by this
     * function. However, a write operation on the other end is never segmented,
     * that is, if the application writes n bytes to the DataOStream, a
     * symmetric read from the DataIStream has at least n bytes available.
     *
     * @param size the number of bytes to advance the buffer
     * @version 1.0
     */
    CO_API const void* getRemainingBuffer( const uint64_t size );

    /**
     * @return the size of the remaining data in the current buffer.
     * @version 1.0
     */
    CO_API uint64_t getRemainingBufferSize();

    /** @internal @return true if any data was read. */
    bool wasUsed() const;

    /** @return true if not all data has been read. @version 1.0 */
    bool hasData() { return _checkBuffer(); }

    /** @return the provider of the istream. */
    CO_API virtual NodePtr getMaster() = 0;
    //@}

protected:
    /** @name Internal */
    //@{
    CO_API DataIStream( const bool swap );
    DataIStream( const DataIStream& );
    CO_API virtual ~DataIStream();

    virtual bool getNextBuffer( uint32_t& compressor, uint32_t& nChunks,
                                const void** chunkData, uint64_t& size )=0;
    //@}

private:
    detail::DataIStream* const _impl;

    /** Read a number of bytes from the stream into a buffer. */
    CO_API void _read( void* data, uint64_t size );

    /**
     * Check that the current buffer has data left, get the next buffer is
     * necessary, return false if no data is left.
     */
    CO_API bool _checkBuffer();
    CO_API void _reset();

    const uint8_t* _decompress( const void* data, const uint32_t name,
                                const uint32_t nChunks,
                                const uint64_t dataSize );

    /** Read a vector of trivial data. */
    template< class T >
    DataIStream& _readFlatVector ( std::vector< T >& value )
        {
            uint64_t nElems = 0;
            *this >> nElems;
            LBASSERTINFO( nElems < LB_BIT48,
                          "Out-of-sync co::DataIStream: " << nElems << " elements?" );
            value.resize( size_t( nElems ));
            if( nElems > 0 )
                *this >> Array< T >( &value.front(), nElems );
            return *this;
        }

    /** Byte-swap a plain data item. @version 1.0 */
    template< class T > void _swap( T& value ) const
        { if( isSwapping( )) swap( value ); }

    /** Byte-swap a C array. @version 1.0 */
    template< class T > void _swap( Array< T > array ) const
        {
            if( !isSwapping( ))
                return;
#pragma omp parallel for
            for( ssize_t i = 0; i < ssize_t( array.num ); ++i )
                swap( array.data[i] );
        }
};
}

#include "dataIStream.ipp" // template implementation

#endif //CO_DATAISTREAM_H
