
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

#ifndef CO_CUSTOMCOMMAND_H
#define CO_CUSTOMCOMMAND_H

#include <co/iCommand.h>   // base class

namespace co
{

namespace detail { class CustomCommand; }

/** A command specialization for custom commands. */
class CustomCommand : public ICommand
{
public:
    /** @internal */
    CO_API CustomCommand( const ICommand& command );

    /** Copy-construct a custom command. */
    CO_API CustomCommand( const CustomCommand& rhs );

    /** Destruct a custom command. */
    CO_API virtual ~CustomCommand();

    /** @internal @return the custom command identifier. */
    CO_API const uint128_t& getCommandID() const;

private:
    CustomCommand();
    CustomCommand& operator = ( const CustomCommand& );
    detail::CustomCommand* const _impl;

    void _init();
};

CO_API std::ostream& operator << ( std::ostream& os, const CustomCommand& );

}

#endif //CO_CUSTOMCOMMAND_H
