
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

#ifndef CO_OBJECTICOMMAND_H
#define CO_OBJECTICOMMAND_H

#include <co/command.h>   // base class


namespace co
{

enum ObjectCommands
{
    CMD_OBJECT_INSTANCE,
    CMD_OBJECT_DELTA,
    CMD_OBJECT_SLAVE_DELTA,
    CMD_OBJECT_MAX_VERSION
    // check that not more then CMD_OBJECT_CUSTOM have been defined!
};

namespace detail { class ObjectCommand; }

// #145 Documentation & API
/** A DataIStream based command for co::Object. */
class ObjectCommand : public Command
{
public:
    /** @internal */
    CO_API ObjectCommand( BufferPtr buffer );

    ObjectCommand( const ObjectCommand& rhs );

    ObjectCommand& operator = ( const ObjectCommand& rhs );

    CO_API virtual ~ObjectCommand();

    const UUID& getObjectID() const;

    uint32_t getInstanceID() const;

private:
    detail::ObjectCommand* const _impl;

};

}

#endif //CO_OBJECTICOMMAND_H
