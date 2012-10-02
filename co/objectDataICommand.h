
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

#include <co/objectICommand.h>   // base class


namespace co
{

namespace detail { class ObjectDataICommand; }

/** @internal A command specialization for object data. */
class ObjectDataICommand : public ObjectICommand
{
public:
    CO_API ObjectDataICommand( const ICommand& command );

    CO_API ObjectDataICommand( LocalNodePtr local, NodePtr remote,
                              ConstBufferPtr buffer, const bool swap );

    ObjectDataICommand( const ObjectDataICommand& rhs );

    CO_API ~ObjectDataICommand();

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
    ObjectDataICommand();
    ObjectDataICommand& operator = ( const ObjectDataICommand& );
    detail::ObjectDataICommand* const _impl;

    void _init();
};

CO_API std::ostream& operator << ( std::ostream& os, const ObjectDataICommand& );

}

#endif //CO_OBJECTDATACOMMAND_H
