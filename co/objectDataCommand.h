
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

#ifndef CO_OBJECTDATACOMMAND_H
#define CO_OBJECTDATACOMMAND_H

#include <co/objectCommand.h>   // base class


namespace co
{

namespace detail { class ObjectDataCommand; }

/** */
class ObjectDataCommand : public ObjectCommand
{
public:
    CO_API ObjectDataCommand( BufferPtr buffer );
    CO_API ~ObjectDataCommand();

    virtual uint128_t getVersion() const;

    uint32_t getSequence() const;

    CO_API uint64_t getDataSize() const;

    CO_API uint32_t getCompressor() const;

    CO_API uint32_t getChunks() const;

    CO_API bool isLast() const;

private:
    detail::ObjectDataCommand* const _impl;
};
}

#endif //CO_OBJECTDATACOMMAND_H
