
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2012-2014, Stefan.Eilemann@epfl.ch
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

#ifndef CO_OBJECTDATAOCOMMAND_H
#define CO_OBJECTDATAOCOMMAND_H

#include <co/objectOCommand.h>   // base class
#include <co/objectICommand.h>    // CMD enums


/** @cond IGNORE */
int testMain( int, char ** );
/** @endcond */

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
     * @param data a pointer to the uncompressed object data.
     * @param size the size of the uncompressed object data.
     * @param isLast true if this is the last command for one object
     * @param stream the stream containing the (possible) compressed object data
     */
    CO_API ObjectDataOCommand( const Connections& receivers,
                               const uint32_t cmd, const uint32_t type,
                               const uint128_t& id, const uint32_t instanceID,
                               const uint128_t& version,
                               const uint32_t sequence,
                               const void* data, const uint64_t size,
                               const bool isLast,
                               DataOStream* stream );

    /** @internal */
    CO_API ObjectDataOCommand( const ObjectDataOCommand& rhs );

    /** Send or dispatch this command during destruction. */
    CO_API virtual ~ObjectDataOCommand();

private:
    ObjectDataOCommand();
    ObjectDataOCommand& operator = ( const ObjectDataOCommand& );
    detail::ObjectDataOCommand* const _impl;

    CO_API ObjectDataICommand _getCommand( LocalNodePtr node ); // needed by:
    friend int ::testMain( int, char ** );

    void _init( const uint128_t& version, const uint32_t sequence,
                const uint64_t dataSize, const bool isLast );
};
}

#endif //CO_OBJECTDATAOCOMMAND_H
