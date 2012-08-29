
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

#ifndef CO_OBJECTOCOMMAND_H
#define CO_OBJECTOCOMMAND_H

#include <co/nodeOCommand.h>   // base class


namespace co
{

namespace detail { class ObjectOCommand; }

/** A DataOStream based command for co::Object. */
class ObjectOCommand : public NodeOCommand
{
public:
    CO_API ObjectOCommand( const Connections& receivers, const uint32_t cmd,
                           const uint32_t type, const UUID& id,
                           const uint32_t instanceID );

    CO_API virtual ~ObjectOCommand();

private:
    detail::ObjectOCommand* const _impl;
};
}

#endif //CO_OBJECTOCOMMAND_H
