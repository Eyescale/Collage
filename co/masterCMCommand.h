
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

#ifndef CO_MASTERCMCOMMAND_H
#define CO_MASTERCMCOMMAND_H

#include <co/iCommand.h>   // base class

namespace co
{
namespace detail { class MasterCMCommand; }

/** @internal A command specialization for masterCM commands. */
class MasterCMCommand : public ICommand
{
public:
    MasterCMCommand( const ICommand& command );

    MasterCMCommand( const MasterCMCommand& rhs );

    virtual ~MasterCMCommand();

    const uint128_t& getRequestedVersion() const;

    const uint128_t& getMinCachedVersion() const;

    const uint128_t& getMaxCachedVersion() const;

    const uint128_t& getObjectID() const;

    uint64_t getMaxVersion() const;

    uint32_t getRequestID() const;

    uint32_t getInstanceID() const;

    uint32_t getMasterInstanceID() const;

    bool useCache() const;

private:
    MasterCMCommand();
    MasterCMCommand& operator = ( const MasterCMCommand& );
    detail::MasterCMCommand* const _impl;

    void _init();
};

}

#endif //CO_MASTERCMCOMMAND_H
