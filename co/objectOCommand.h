
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2013-2014, Stefan.Eilemann@epfl.ch
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

#ifndef CO_OBJECTOCOMMAND_H
#define CO_OBJECTOCOMMAND_H

#include <co/oCommand.h>   // base class

namespace co
{
namespace detail { class ObjectOCommand; }

/**
 * A class for sending commands and data to local & external objects.
 *
 * @sa co::OCommand
 */
class ObjectOCommand : public OCommand
{
public:
    /** @internal
     * Construct a command which is send & dispatched to a co::Object.
     *
     * @param receivers list of connections where to send the command to.
     * @param cmd the command.
     * @param type the command type for dispatching.
     * @param id the ID of the object to dispatch this command to.
     * @param instanceID the instance of the object to dispatch the command to.
     */
    CO_API ObjectOCommand( const Connections& receivers, const uint32_t cmd,
                           const uint32_t type, const uint128_t& id,
                           const uint32_t instanceID );

    /** @internal
     * Construct a command which is dispatched locally to a co::Object.
     *
     * @param dispatcher the dispatcher to dispatch this command.
     * @param localNode the local node that holds the command cache.
     * @param cmd the command.
     * @param type the command type for dispatching.
     * @param id the ID of the object to dispatch this command to.
     * @param instanceID the instance of the object to dispatch the command to.
     */
    CO_API ObjectOCommand( Dispatcher* const dispatcher, LocalNodePtr localNode,
                           const uint32_t cmd, const uint32_t type,
                           const uint128_t& id, const uint32_t instanceID );

    /** @internal */
    CO_API ObjectOCommand( const ObjectOCommand& rhs );

    /** Send or dispatch this command during destruction. @version 1.0 */
    CO_API virtual ~ObjectOCommand();

private:
    ObjectOCommand();
    ObjectOCommand& operator = ( const ObjectOCommand& );
    detail::ObjectOCommand* const _impl;

    void _init( const uint128_t& id, const uint32_t instanceID );
};
}

#endif //CO_OBJECTOCOMMAND_H
