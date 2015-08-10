
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_CUSTOMICOMMAND_H
#define CO_CUSTOMICOMMAND_H

#include <co/iCommand.h>   // base class

namespace co
{

namespace detail { class CustomICommand; }

/**
 * An input command specialization for custom commands.
 * @sa LocalNode::registerCommandHandler()
 */
class CustomICommand : public ICommand
{
public:
    /** @internal */
    CO_API explicit CustomICommand( const ICommand& command );

    /** Copy-construct a custom command. */
    CO_API CustomICommand( const CustomICommand& rhs );

    /** Destruct a custom command. */
    CO_API virtual ~CustomICommand();

    /** @internal @return the custom command identifier. */
    CO_API const uint128_t& getCommandID() const;

private:
    CustomICommand();
    CustomICommand& operator = ( const CustomICommand& );
    detail::CustomICommand* const _impl;

    void _init();
};

CO_API std::ostream& operator << ( std::ostream& os, const CustomICommand& );

}

#endif //CO_CUSTOMICOMMAND_H
