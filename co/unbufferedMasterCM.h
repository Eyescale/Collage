
/* Copyright (c) 2007-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_UNBUFFEREDMASTERCM_H
#define CO_UNBUFFEREDMASTERCM_H

#include "versionedMasterCM.h"           // base class

namespace co
{
/**
 * @internal
 * An object change manager handling versioned objects without any
 * buffering.
 */
class UnbufferedMasterCM : public VersionedMasterCM
{
public:
    explicit UnbufferedMasterCM( Object* object );
    virtual ~UnbufferedMasterCM();

    /** @name Versioning */
    //@{
    uint128_t commit( const uint32_t incarnation ) override;
    void setAutoObsolete( const uint32_t ) override {}
    uint32_t getAutoObsolete() const override { return 0; }
    //@}

private:
    /* The command handlers. */
    bool _cmdCommit( ICommand& pkg );
};
}

#endif // CO_UNBUFFEREDMASTERCM_H
