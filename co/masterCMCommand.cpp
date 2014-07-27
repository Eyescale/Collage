
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2014, Stefan.Eilemann@epfl.ch
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

#include "masterCMCommand.h"


namespace co
{

namespace detail
{

class MasterCMCommand
{
public:
    MasterCMCommand() : useCache( false ) {}

    MasterCMCommand( const MasterCMCommand& ) : useCache( false ) {}

    uint128_t requestedVersion;
    uint128_t minCachedVersion;
    uint128_t maxCachedVersion;
    uint128_t objectID;
    uint64_t maxVersion;
    uint32_t requestID;
    uint32_t instanceID;
    uint32_t masterInstanceID;
    bool useCache;
};

}

MasterCMCommand::MasterCMCommand( const ICommand& command )
    : ICommand( command )
    , _impl( new detail::MasterCMCommand )
{
    _init();
}

MasterCMCommand::MasterCMCommand( const MasterCMCommand& rhs )
    : ICommand( rhs )
    , _impl( new detail::MasterCMCommand( *rhs._impl ))
{
    _init();
}

void MasterCMCommand::_init()
{
    if( isValid( ))
        *this >> _impl->requestedVersion >> _impl->minCachedVersion
              >> _impl->maxCachedVersion >> _impl->objectID >> _impl->maxVersion
              >> _impl->requestID >> _impl->instanceID
              >> _impl->masterInstanceID >> _impl->useCache;
}

MasterCMCommand::~MasterCMCommand()
{
    delete _impl;
}

const uint128_t& MasterCMCommand::getRequestedVersion() const
{
    return _impl->requestedVersion;
}

const uint128_t& MasterCMCommand::getMinCachedVersion() const
{
    return _impl->minCachedVersion;
}

const uint128_t& MasterCMCommand::getMaxCachedVersion() const
{
    return _impl->maxCachedVersion;
}

const uint128_t& MasterCMCommand::getObjectID() const
{
    return _impl->objectID;
}

uint64_t MasterCMCommand::getMaxVersion() const
{
    return _impl->maxVersion;
}

uint32_t MasterCMCommand::getRequestID() const
{
    return _impl->requestID;
}

uint32_t MasterCMCommand::getInstanceID() const
{
    return _impl->instanceID;
}

uint32_t MasterCMCommand::getMasterInstanceID() const
{
    return _impl->masterInstanceID;
}

bool MasterCMCommand::useCache() const
{
    return _impl->useCache;
}

}
