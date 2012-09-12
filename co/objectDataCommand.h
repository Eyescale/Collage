
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
    CO_API ObjectDataCommand( const Command& command );

    CO_API ObjectDataCommand( ConstBufferPtr buffer );

    ObjectDataCommand( const ObjectDataCommand& rhs );

    CO_API ~ObjectDataCommand();

    /** @return the object version. */
    virtual uint128_t getVersion() const;

    /** @return the index in a sequence of commands. */
    uint32_t getSequence() const;

    /** @return the size of the packed object data. */
    CO_API uint64_t getDataSize() const;

    /** @return the compressor used for the object data. */
    CO_API uint32_t getCompressor() const;

    /** @return the number of chunks containing the object data. */
    CO_API uint32_t getChunks() const;

    /** @return true if this is the last command for one object. */
    CO_API bool isLast() const;

private:
    ObjectDataCommand();
    ObjectDataCommand& operator = ( const ObjectDataCommand& );
    detail::ObjectDataCommand* const _impl;

    void _init();
};

CO_API std::ostream& operator << ( std::ostream& os, const ObjectDataCommand& );

}

#endif //CO_OBJECTDATACOMMAND_H
