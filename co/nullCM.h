
/* Copyright (c) 2007-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *               2011-2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef EQNET_NULLCM_H
#define EQNET_NULLCM_H

#include "objectCM.h"    // base class

namespace co
{
    class Node;

    /**
     * The NOP object change manager for unmapped objects.
     * @internal
     */
    class NullCM : public ObjectCM
    {
    public:
        NullCM() : ObjectCM( 0 ) {}
        virtual ~NullCM() {}

        virtual void init() override {}

        virtual void push( const uint128_t&, const uint128_t&,
                           const Nodes& ) override { LBDONTCALL; }

        virtual uint128_t getHeadVersion() const override
            { return VERSION_NONE; }
        virtual uint128_t getVersion() const override { return VERSION_NONE; }
        virtual bool isMaster() const override { return false; }
        virtual uint32_t getMasterInstanceID() const override
            { LBDONTCALL; return EQ_INSTANCE_INVALID; }

        virtual void addSlave( MasterCMCommand command ) override { LBDONTCALL; }
        virtual void removeSlaves( NodePtr ) override { LBDONTCALL; }

    private:
    };
}

#endif // EQNET_NULLCM_H
