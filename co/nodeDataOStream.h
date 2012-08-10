
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

#ifndef CO_NODEDATAOSTREAM_H
#define CO_NODEDATAOSTREAM_H

#include <co/dataOStream.h>   // base class


namespace co
{

namespace detail { class NodeDataOStream; }

/** The DataOStream for node data & commands. */
class NodeDataOStream : public DataOStream
{
public:
    NodeDataOStream( ConnectionPtr connection, uint32_t type, uint32_t cmd );
    NodeDataOStream( NodeDataOStream const& rhs );
    virtual ~NodeDataOStream();

protected:
    virtual void sendData( const void* buffer, const uint64_t size,
                           const bool last );

private:
    detail::NodeDataOStream* const _impl;
};
}

#endif //CO_NODEDATAOSTREAM_H
