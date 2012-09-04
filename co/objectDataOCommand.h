
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

/**
 * @internal
 * A class for sending commands & object data to distributed objects.
 *
 * @sa co::ObjectOCommand
 */
class ObjectDataOCommand : public ObjectOCommand
{
public:
    /**
     * Construct a command which is send & dispatched to a co::Object.
     *
     * @param receivers list of connections where to send the command to.
     * @param cmd the command.
     * @param type the command type for dispatching.
     * @param id the ID of the object to dispatch this command to.
     * @param instanceID the instance of the object to dispatch the command to.
     * @param version the version of the object data.
     * @param sequence the index in a sequence of a set commands.
     * @param dataSize the size of the packed, uncompressed object data.
     * @param isLast true if this is the last command for one object
     * @param stream the stream containing the (possible) compressed object data
     */
    CO_API ObjectDataOCommand( const Connections& receivers,
                               const uint32_t cmd, const uint32_t type,
                               const UUID& id, const uint32_t instanceID,
                               const uint128_t& version,
                               const uint32_t sequence,
                               const uint64_t dataSize, const bool isLast,
                               DataOStream* stream );

    /** Send or dispatch this command during destruction. */
    CO_API virtual ~ObjectDataOCommand();

    /** Adds additional data to this command. */
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
