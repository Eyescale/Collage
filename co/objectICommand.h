
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

#ifndef CO_OBJECTICOMMAND_H
#define CO_OBJECTICOMMAND_H

#include <co/iCommand.h>   // base class

namespace co
{
namespace detail { class ObjectICommand; }

/** An input command specialization for objects. */
class ObjectICommand : public ICommand
{
public:
    /** @internal */
    CO_API ObjectICommand( LocalNodePtr local, NodePtr remote,
                           ConstBufferPtr buffer, const bool swap );

    /** Copy-construct an object command from a generic ICommand. @version 1.0*/
    CO_API ObjectICommand( const ICommand& command );

    /** Copy-construct an object command. */
    CO_API ObjectICommand( const ObjectICommand& rhs );

    /** Destruct an object command. @version 1.0 */
    CO_API virtual ~ObjectICommand();

    /** @internal @return the object adressed by this command. */
    const uint128_t& getObjectID() const;

    /** @internal @return the object instance adressed by this command. */
    uint32_t getInstanceID() const;

private:
    ObjectICommand();
    ObjectICommand& operator = ( const ObjectICommand& );
    detail::ObjectICommand* const _impl;

    void _init();
};

/** Output information about the object input command. @version 1.0 */
CO_API std::ostream& operator << ( std::ostream& os, const ObjectICommand& );
}

#endif //CO_OBJECTICOMMAND_H
