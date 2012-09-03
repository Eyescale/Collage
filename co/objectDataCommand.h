
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

/** @internal A command specialization for object data. */
class ObjectDataCommand : public ObjectCommand
{
public:
    /** @internal Construct an object data command from a base command. */
    CO_API ObjectDataCommand( const Command& command );

    /** @internal */
    CO_API ~ObjectDataCommand();

    /** @internal @return the object version. */
    virtual uint128_t getVersion() const;

    /** @internal @return the index in a sequence of commands. */
    uint32_t getSequence() const;

    /** @internal @return the size of the packed object data. */
    CO_API uint64_t getDataSize() const;

    /** @internal @return the compressor used for the object data. */
    CO_API uint32_t getCompressor() const;

    /** @internal @return the number of chunks containing the object data. */
    CO_API uint32_t getChunks() const;

    /** @internal @return true if this is the last command for one object. */
    CO_API bool isLast() const;

private:
    detail::ObjectDataCommand* const _impl;

    void _init();
};

CO_API std::ostream& operator << ( std::ostream& os, const ObjectDataCommand& );

}

#endif //CO_OBJECTDATACOMMAND_H
