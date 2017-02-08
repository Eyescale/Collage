
/* Copyright (c) 2007-2017, Stefan Eilemann <eile@equalizergraphics.com>
 *                          Cedric Stalder <cedric.stalder@gmail.com>
 *                          Daniel Nachbaur <danielnachbaur@gmail.com>
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
#include <co/types.h>
#include <lunchbox/array.h> // used inline
#include <lunchbox/bitOperation.h>

#include <boost/type_traits.hpp>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace co
{
namespace detail
{
class DataIStream;
}

/** A std::istream-like input data stream for binary data. */
class DataIStream
{
public:
    /** @name Internal */
    //@{
    /** @internal @return the number of remaining buffers. */
    virtual size_t nRemainingBuffers() const = 0;

    virtual uint128_t getVersion() const = 0; //!< @internal
    virtual void reset() { _reset(); }        //!< @internal
    //@}

    /** @name Data input */
    //@{
    /** @return a value from the stream. @version 1.0 */
    template <typename T>
    T read()
    {
        T value;
        *this >> value;
        return value;
    }

    /** Read a plain data item. @version 1.0 */
    template <class T>
    DataIStream& operator>>(T& value)
    {
        _read(value, boost::has_trivial_copy<T>());
        return *this;
    }

    /** Read a C array. @version 1.0 */
    template <class T>
    DataIStream& operator>>(Array<T> array)
    {
        _readArray(array, boost::has_trivial_copy<T>());
        return *this;
    }

    /**
     * Read a lunchbox::RefPtr. Refcount has to managed by caller.
     * @version 1.1
     */
    template <class T>
    DataIStream& operator>>(lunchbox::RefPtr<T>&);

    /** Read a lunchbox::Buffer. @version 1.0 */
    template <class T>
    DataIStream& operator>>(lunchbox::Buffer<T>&);

    /** Read a std::vector of serializable items. @version 1.0 */
    template <class T>
    DataIStream& operator>>(std::vector<T>&);

    /** Read a std::map of serializable items. @version 1.0 */
    template <class K, class V>
    DataIStream& operator>>(std::map<K, V>&);

    /** Read a std::set of serializable items. @version 1.0 */
    template <class T>
    DataIStream& operator>>(std::set<T>&);

    /** Read an unordered_map of serializable items. @version 1.0 */
    template <class K, class V>
    DataIStream& operator>>(std::unordered_map<K, V>&);

    /** Read an unordered_set of serializable items. @version 1.0 */
    template <class T>
    DataIStream& operator>>(std::unordered_set<T>&);

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
    template <typename O, typename C>
    void deserializeChildren(O* object, const std::vector<C*>& old,
                             std::vector<C*>& result);

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
    CO_API const void* getRemainingBuffer(const uint64_t size);

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
    CO_API virtual NodePtr getRemoteNode() const = 0;

    /** @return the receiver of the istream. */
    CO_API virtual LocalNodePtr getLocalNode() const = 0;
    //@}

protected:
    /** @name Internal */
    //@{
    CO_API DataIStream();
    CO_API virtual ~DataIStream();

    virtual bool getNextBuffer(CompressorInfo& info, uint32_t& nChunks,
                               const void*& chunkData, uint64_t& size) = 0;
    //@}

private:
    detail::DataIStream* const _impl;

    /** Read a number of bytes from the stream into a buffer. */
    CO_API void _read(void* data, uint64_t size);

    /**
     * Check that the current buffer has data left, get the next buffer is
     * necessary, return false if no data is left.
     */
    CO_API bool _checkBuffer();
    CO_API void _reset();

    const uint8_t* _decompress(const void* data, const CompressorInfo& info,
                               uint32_t nChunks, uint64_t dataSize);

    /** Read a vector of trivial data. */
    template <class T>
    DataIStream& _readFlatVector(std::vector<T>& value)
    {
        uint64_t nElems = 0;
        *this >> nElems;
        LBASSERTINFO(nElems < LB_BIT48,
                     "Out-of-sync DataIStream: " << nElems << " elements?");
        value.resize(size_t(nElems));
        if (nElems > 0)
            *this >> Array<T>(&value.front(), nElems);
        return *this;
    }

    /** Read a plain data item. */
    template <class T>
    void _read(T& value, const boost::true_type&)
    {
        _read(&value, sizeof(value));
    }

    /** Read a non-plain data item. */
    template <class T>
    void _read(T& value, const boost::false_type&)
    {
        _readSerializable(value, boost::is_base_of<servus::Serializable, T>());
    }

    /** Read a serializable object. */
    template <class T>
    void _readSerializable(T& value, const boost::true_type&);

    /** Read an Array of POD data */
    template <class T>
    void _readArray(Array<T>, const boost::true_type&);

    /** Read an Array of non-POD data */
    template <class T>
    void _readArray(Array<T>, const boost::false_type&);
};
}

#include "dataIStream.ipp" // template implementation

#endif // CO_DATAISTREAM_H
