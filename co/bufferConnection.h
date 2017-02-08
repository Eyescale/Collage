
/* Copyright (c) 2007-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_BUFFER_CONNECTION_H
#define CO_BUFFER_CONNECTION_H

#include <co/connection.h> // base class
#include <lunchbox/types.h>

namespace co
{
namespace detail
{
class BufferConnection;
}

/** A proxy connection buffering outgoing data into a memory buffer. */
class BufferConnection : public Connection
{
public:
    /** Construct a new buffer connection. @version 1.0 */
    CO_API BufferConnection();

    /** Destruct this buffer connection. @version 1.0 */
    CO_API virtual ~BufferConnection();

    /**
     * Flush the accumulated data, sending it to the given connection.
     * @version 1.0
     */
    CO_API void sendBuffer(ConnectionPtr connection);

    /** @return the internal data buffer. @version 1.0 */
    CO_API const lunchbox::Bufferb& getBuffer() const;

    /** @return the internal data buffer. @version 1.0 */
    CO_API lunchbox::Bufferb& getBuffer();

    /** @return the size of the accumulated data. @version 1.0 */
    CO_API uint64_t getSize() const;

protected:
    /** @internal */
    //@{
    void readNB(void*, const uint64_t) override { LBDONTCALL; }
    int64_t readSync(void*, const uint64_t, const bool) override
    {
        LBDONTCALL;
        return -1;
    }
    CO_API int64_t write(const void* buffer, const uint64_t bytes) override;

    Notifier getNotifier() const override
    {
        LBDONTCALL;
        return 0;
    }
    //@}

private:
    detail::BufferConnection* const _impl;
};

typedef lunchbox::RefPtr<BufferConnection> BufferConnectionPtr;
typedef lunchbox::RefPtr<const BufferConnection> ConstBufferConnectionPtr;
}

#endif // CO_BUFFER_CONNECTION_H
