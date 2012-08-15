
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

#ifndef CO_OBJECTDATAICOMMAND_H
#define CO_OBJECTDATAICOMMAND_H

#include <co/objectICommand.h>   // base class
#include <co/objectCommand.h>    // CMD enums


namespace co
{

namespace detail { class ObjectDataICommand; }

/** */
class ObjectDataICommand : public ObjectICommand
{
public:
    ObjectDataICommand( CommandPtr command );
    virtual ~ObjectDataICommand();

    virtual uint128_t getVersion() const;

    uint32_t getSequence() const;

    uint64_t getDataSize() const;

    uint32_t getCompressor() const;

    uint32_t getChunks() const;

    bool isLast() const;

private:
    detail::ObjectDataICommand* const _impl;
};
}

#endif //CO_OBJECTDATAICOMMAND_H
