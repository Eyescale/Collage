
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

#ifndef CO_NODEOCOMMAND_H
#define CO_NODEOCOMMAND_H

#include <co/dataOStream.h>   // base class


namespace co
{

namespace detail { class NodeOCommand; }

/** A DataOStream based command for co::Node. */
class NodeOCommand : public DataOStream
{
public:
    NodeOCommand( const Connections& connections, const uint32_t type,
                  const uint32_t cmd );

    NodeOCommand( NodeOCommand const& rhs );

    virtual ~NodeOCommand();

protected:
    virtual void sendData( const void* buffer, const uint64_t size,
                           const bool last );

private:
    detail::NodeOCommand* const _impl;

    void _init();
};
}

#endif //CO_NODEOCOMMAND_H
