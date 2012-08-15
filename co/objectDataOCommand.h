
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

#ifndef CO_OBJECTDATAOCOMMAND_H
#define CO_OBJECTDATAOCOMMAND_H

#include <co/objectOCommand.h>   // base class
#include <co/objectCommand.h>    // CMD enums


namespace co
{

namespace detail { class ObjectDataOCommand; }

/** A DataOStream based command for co::ObjectDataOStream. */
class ObjectDataOCommand : protected ObjectOCommand
{
public:
    ObjectDataOCommand( const Connections& receivers, const uint32_t type,
                        const uint32_t cmd, const UUID& id,
                        const uint32_t instanceID, const uint128_t& version,
                        const uint32_t sequence, const uint64_t dataSize,
                        const bool isLast, const void* buffer,
                        DataOStream* stream );

    virtual ~ObjectDataOCommand();

    template< typename T > ObjectDataOCommand& operator << ( const T& value )
        { _addUserData( &value, sizeof( value )); return *this; }

protected:
    virtual void sendData( const void* buffer, const uint64_t size,
                           const bool last );

private:
    detail::ObjectDataOCommand* const _impl;

    void _addUserData( const void* data, uint64_t size );
};
}

#endif //CO_OBJECTDATAOCOMMAND_H
