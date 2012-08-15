
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
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

#ifndef CO_DATAOSTREAM_H
#define CO_DATAOSTREAM_H

#include <co/api.h>
#include <co/array.h> // used inline
#include <co/types.h>
#include <lunchbox/nonCopyable.h> // base class

#include <iostream>
#include <vector>

namespace co
{
namespace detail { class DataOStream; }
namespace DataStreamTest { class Sender; }

    /**
     * A std::ostream-like interface for object serialization.
     *
     * Implements buffering, retaining and compressing data in a binary format.
     * Derived classes send the data using the appropriate command packets.
     */
    class DataOStream : public lunchbox::NonCopyable
    {
    public:
        /** @name Internal */
        //@{
        /** @internal Disable and flush the output to the current receivers. */
        CO_API void disable();

        /** @internal Disable, then send the packet to the current receivers. */
        void disable( const Packet& packet );

        /** @internal Enable copying of all data into a saved buffer. */
        void enableSave();

        /** @internal Disable copying of all data into a saved buffer. */
        void disableSave();

        /** @internal @return if data was sent since the last enable() */
        CO_API bool hasSentData() const;

        /** @internal */
        const Connections& getConnections() const;

        /** @internal */
        uint32_t getCompressor() const;

        /** @internal */
        uint32_t getNumChunks() const;

        /**
         * Collect compressed data.
         * @return the total size of the compressed data.
         * @internal
         */
        CO_API uint64_t getCompressedData( void** chunks,
                                            uint64_t* chunkSizes ) const;
        //@}

        /** @name Data output */
        //@{
        /** Write a plain data item by copying it to the stream. @version 1.0 */
        template< typename T > DataOStream& operator << ( const T& value )
            { _write( &value, sizeof( value )); return *this; }

        /** Write a C array. @version 1.0 */
        template< typename T > DataOStream& operator << ( Array< T > array )
            { _write( array.data, array.getNumBytes( )); return *this; }

        /** Write a std::vector of serializable items. @version 1.0 */
        template< typename T >
        DataOStream& operator << ( const std::vector< T >& value );

        /** @internal
         * Serialize child objects.
         *
         * The DataIStream has a deserialize counterpart to this method. All
         * child objects have to be registered or mapped beforehand.
         */
        template< typename C >
        void serializeChildren( const std::vector< C* >& children );
        //@}

    protected:
        CO_API DataOStream(); //!< @internal
        virtual CO_API ~DataOStream(); //!< @internal

        /** @internal Initialize the given compressor. */
        void _initCompressor( const uint32_t compressor );

        /** @internal Enable output. */
        CO_API void _enable();

        /** @internal Flush remaining data in the buffer. */
        void _flush();

        /** @internal
         * Set up the connection list for a group of  nodes, using multicast
         * where possible.
         */
        void _setupConnections( const Nodes& receivers );

        void _setupConnections( const Connections& connections );

        /** @internal Set up the connection (list) for one node. */
        void _setupConnection( NodePtr node, const bool useMulticast );

        /** @internal Needed by unit test. */
        CO_API void _setupConnection( ConnectionPtr connection );
        friend class DataStreamTest::Sender;

        /** @internal Resend the saved buffer to all enabled connections. */
        void _resend();

        void _send( const Packet& packet ); //!< @internal
        void _clearConnections(); //!< @internal

        /** @internal @name Packet sending, used by the subclasses */
        //@{
        /** @internal Send a data buffer (packet) to the receivers. */
        virtual void sendData( const void* buffer, const uint64_t size,
                               const bool last ) = 0;

        //@}

        /** @internal Reset the whole stream. */
        virtual CO_API void reset();

    private:
        detail::DataOStream* const _impl;

        /** Write a number of bytes from data into the stream. */
        CO_API void _write( const void* data, uint64_t size );

        bool _disable();

        /** Helper function preparing data for sendData() as needed. */
        void _sendData( const void* data, const uint64_t size );

        /** Reset after sending a buffer. */
        void _resetBuffer();

        /** Write a vector of trivial data. */
        template< typename T >
        DataOStream& _writeFlatVector( const std::vector< T >& value )
        {
            const uint64_t nElems = value.size();
            _write( &nElems, sizeof( nElems ));
            if( nElems > 0 )
                _write( &value.front(), nElems * sizeof( T ));
            return *this;
        }
        /** Send the trailing data (packet) to the receivers */
        void _sendFooter( const void* buffer, const uint64_t size );
    };

    std::ostream& operator << ( std::ostream&, const DataOStream& );

}

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
    /** Write a std::vector of serializable items. */
    template< typename T > inline DataOStream&
    DataOStream::operator << ( const std::vector< T >& value )
    {
        const uint64_t nElems = value.size();
        (*this) << nElems;
        for( uint64_t i = 0; i < nElems; ++i )
            (*this) << value[i];
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
#endif //CO_DATAOSTREAM_H
