
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

#ifndef CO_CUSTOMOCOMMAND_H
#define CO_CUSTOMOCOMMAND_H

#include <co/nodeOCommand.h>    // base class


namespace co
{

namespace detail { class CustomOCommand; }

/**
 * A class for sending custom commands and data to local & external nodes.
 *
 * @sa co::NodeOCommand
 */
class CustomOCommand : public NodeOCommand
{
public:
    /**
     * Construct a command dispatched to a remote LocalNode custom command
     * handler.
     *
     * @param receivers list of connections where to send the command to.
     * @param commandID the custom command identifier
     */
    CO_API CustomOCommand( const Connections& receivers,
                           const uint128_t& commandID );

    /**
     * Construct a command dispatched to a local custom command handler.
     *
     * @param localNode the local node that holds the command cache.
     * @param commandID the custom command identifier
     */
    CO_API CustomOCommand( LocalNodePtr localNode, const uint128_t& commandID );

    CO_API CustomOCommand( const CustomOCommand& rhs ); //!< @internal

    /** Send or dispatch this command during destruction. */
    CO_API virtual ~CustomOCommand();

private:
    CustomOCommand();
    CustomOCommand& operator = ( const CustomOCommand& );
    detail::CustomOCommand* const _impl;

    void _init();
};
}

#endif //CO_CUSTOMOCOMMAND_H
