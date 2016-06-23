
/* Copyright (c) 2007-2016, Stefan Eilemann <eile@equalizergraphics.com>
 *                          Cedric Stalder <cedric.stalder@gmail.com>
 *                          Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_STATICMASTERCM_H
#define CO_STATICMASTERCM_H

#include "objectCM.h"    // base class
#include <co/version.h>  // enum
#include "objectInstanceDataOStream.h"

#include <deque>

namespace co
{
class Node;

/** @internal
 * An object change manager handling a static master instance.
 */
class StaticMasterCM : public ObjectCM
{
public:
    explicit StaticMasterCM( Object* object ) : ObjectCM( object ) {}
    virtual ~StaticMasterCM() {}

    void init() override {}

    /** @name Versioning */
    //@{
    void setAutoObsolete( const uint32_t ) override {}
    uint32_t getAutoObsolete() const override { return 0; }

    uint128_t getHeadVersion() const override { return VERSION_FIRST; }
    uint128_t getVersion() const override { return VERSION_FIRST; }
    //@}

    bool isMaster() const override { return true; }
    uint32_t getMasterInstanceID() const override
    { LBDONTCALL; return CO_INSTANCE_INVALID; }

    bool addSlave( const MasterCMCommand& command ) override
    { return ObjectCM::_addSlave( command, VERSION_FIRST ); }
    void removeSlaves( NodePtr ) override { /* NOP */}
};
}

#endif // CO_STATICMASTERCM_H
