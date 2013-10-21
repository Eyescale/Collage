
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
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

#ifndef CO_DATAOSTREAM_H
#define CO_DATAOSTREAM_H

#include <co/api.h>
#include <co/array.h> // used inline
#include <co/types.h>
#include <lunchbox/nonCopyable.h> // base class
#include <lunchbox/stdExt.h>

#include <map>
#include <set>
#include <vector>

namespace co
{
namespace detail { class DataOStream; }
namespace DataStreamTest { class Sender; }

    /**
     * A std::ostream-like interface for object serialization.
     *
     * Implements buffering, retaining and compressing data in a binary format.
     * Derived classes send the data using the appropriate commands.
     */
    class DataOStream : public lunchbox::NonCopyable
    {
    public:
        /** @name Internal */
        //@{
        /** @internal Disable and flush the output to the current receivers. */
        CO_API void disable();

        /** @internal Enable copying of all data into a saved buffer. */
        void enableSave();

        /** @internal Disable copying of all data into a saved buffer. */
        void disableSave();

        /** @internal @return if data was sent since the last enable() */
        CO_API bool hasSentData() const;

        /** @internal */
        CO_API const Connections& getConnections() const;

        /** @internal Stream the data header (compressor, nChunks). */
        DataOStream& streamDataHeader( DataOStream& os );

        /** @internal Send the (compressed) data using the given connection. */
        void sendBody( ConnectionPtr connection, const void* data,
                       const uint64_t size );

        /** @internal @return the compressed data size, 0 if uncompressed.*/
        uint64_t getCompressedDataSize() const;
        //@}

        /** @name Data output */
        //@{
        /** Write a plain data item by copying it to the stream. @version 1.0 */
        template< class T > DataOStream& operator << ( const T& value )
            { _write( &value, sizeof( value )); return *this; }

        /** Write a C array. @version 1.0 */
        template< class T > DataOStream& operator << ( Array< T > array )
            { _write( array.data, array.getNumBytes( )); return *this; }

        /** Write a lunchbox::Buffer. @version 1.0 */
        template< class T >
        DataOStream& operator << ( const lunchbox::Buffer< T >& buffer );

        /** Write a std::vector of serializable items. @version 1.0 */
        template< class T >
        DataOStream& operator << ( const std::vector< T >& value );

        /** Write a std::map of serializable items. @version 1.0 */
        template< class K, class V >
        DataOStream& operator << ( const std::map< K, V >& value );

        /** Write a std::set of serializable items. @version 1.0 */
        template< class T >
        DataOStream& operator << ( const std::set< T >& value );

        /** Write a stde::hash_map of serializable items. @version 1.0 */
        template< class K, class V >
        DataOStream& operator << ( const stde::hash_map< K, V >& value );

        /** Write a stde::hash_set of serializable items. @version 1.0 */
        template< class T >
        DataOStream& operator << ( const stde::hash_set< T >& value );

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
        DataOStream( DataOStream& rhs );  //!< @internal
        virtual CO_API ~DataOStream(); //!< @internal

        /** @internal */
        CO_API lunchbox::Bufferb& getBuffer();

        /** @internal Initialize the given compressor. */
        void _initCompressor( const uint32_t compressor );

        /** @internal Enable output. */
        CO_API void _enable();

        /** @internal Flush remaining data in the buffer. */
        void flush( const bool last );

        /** @internal
         * Set up the connection list for a group of nodes, using multicast
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

        void _clearConnections(); //!< @internal

        /** @internal @name Data sending, used by the subclasses */
        //@{
        /** @internal Send a data buffer (command) to the receivers. */
        virtual void sendData( const void* buffer, const uint64_t size,
                               const bool last ) = 0;
        //@}

        /** @internal Reset the whole stream. */
        virtual CO_API void reset();

    private:
        detail::DataOStream* const _impl;

        /** Collect compressed data. */
        CO_API uint64_t _getCompressedData( void** chunks,
                                            uint64_t* chunkSizes ) const;

        /** Write a number of bytes from data into the stream. */
        CO_API void _write( const void* data, uint64_t size );

        /** Helper function preparing data for sendData() as needed. */
        void _sendData( const void* data, const uint64_t size );

        /** Reset after sending a buffer. */
        void _resetBuffer();

        /** Write a vector of trivial data. */
        template< class T >
        DataOStream& _writeFlatVector( const std::vector< T >& value )
        {
            const uint64_t nElems = value.size();
            _write( &nElems, sizeof( nElems ));
            if( nElems > 0 )
                _write( &value.front(), nElems * sizeof( T ));
            return *this;
        }
        /** Send the trailing data (command) to the receivers */
        void _sendFooter( const void* buffer, const uint64_t size );
    };

    std::ostream& operator << ( std::ostream&, const DataOStream& );
}

#include "dataOStream.ipp" // template implementation

#endif //CO_DATAOSTREAM_H
