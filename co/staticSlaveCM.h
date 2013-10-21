
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

#ifndef CO_STATICSLAVECM_H
#define CO_STATICSLAVECM_H

#include "objectCM.h"     // base class
#include <co/objectVersion.h> // VERSION_FOO values
#include <lunchbox/thread.h>  // thread-safety check

namespace co
{
    /** @internal
     * An object change manager handling static object slave instances.
     */
    class StaticSlaveCM : public ObjectCM
    {
    public:
        StaticSlaveCM( Object* object );
        virtual ~StaticSlaveCM();

        void init() override {}

        /**
         * @name Versioning
         */
        //@{
        uint128_t getHeadVersion() const override { return VERSION_FIRST; }
        uint128_t getVersion() const override { return VERSION_FIRST; }
        //@}

        bool isMaster() const override { return false; }
        uint32_t getMasterInstanceID() const override
            { return CO_INSTANCE_INVALID; }

        void addSlave( const MasterCMCommand& ) override { LBDONTCALL; }
        void removeSlaves( NodePtr ) override {}

        void applyMapData( const uint128_t& version ) override;
        void addInstanceDatas( const ObjectDataIStreamDeque&,
                                       const uint128_t& startVersion ) override;
    protected:
        /** input stream for receiving the current version */
        ObjectDataIStream* _currentIStream;

    private:
        /* The command handlers. */
        bool _cmdInstance( ICommand& command );
        LB_TS_VAR( _rcvThread );
    };
}

#endif // CO_STATICSLAVECM_H
