
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

#ifndef CO_MASTERCMCOMMAND_H
#define CO_MASTERCMCOMMAND_H

#include <co/command.h>   // base class


namespace co
{

namespace detail { class MasterCMCommand; }

/** @internal A command specialization for masterCM commands. */
class MasterCMCommand : public Command
{
public:
    /** Construct a master CM command from a base command. */
    MasterCMCommand( const Command& command );

    /** @internal */
    MasterCMCommand( const MasterCMCommand& rhs );

    virtual ~MasterCMCommand();

    /** @internal */
    const uint128_t& getRequestedVersion() const;

    /** @internal */
    const uint128_t& getMinCachedVersion() const;

    /** @internal */
    const uint128_t& getMaxCachedVersion() const;

    /** @internal */
    const UUID& getObjectID() const;

    /** @internal */
    uint64_t getMaxVersion() const;

    /** @internal */
    uint32_t getRequestID() const;

    /** @internal */
    uint32_t getInstanceID() const;

    /** @internal */
    uint32_t getMasterInstanceID() const;

    /** @internal */
    bool useCache() const;

private:
    MasterCMCommand();
    MasterCMCommand& operator = ( const MasterCMCommand& );
    detail::MasterCMCommand* const _impl;

    void _init();
};

}

#endif //CO_MASTERCMCOMMAND_H
