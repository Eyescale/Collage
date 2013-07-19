
/* Copyright (c) 2010-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_MASTERCM_H
#define CO_MASTERCM_H

#include "objectCM.h" // base class
#include "dataIStreamQueue.h" // member
#include <co/types.h>

#include <lunchbox/mtQueue.h> // member
#include <lunchbox/pool.h>    // member
#include <lunchbox/stdExt.h>  // member
#include <lunchbox/thread.h>  // thread-safety check

namespace co
{
    /**
     * @internal
     * The base class for versioned master change managers.
     */
    class VersionedMasterCM : public ObjectCM
    {
    protected:
        typedef lunchbox::ScopedWrite Mutex;

    public:
        VersionedMasterCM( Object* object );
        virtual ~VersionedMasterCM();

        void init() override {}

        /** @name Versioning */
        //@{
        uint128_t sync( const uint128_t& version ) override;

        uint128_t getHeadVersion() const override
            { Mutex mutex( _slaves ); return _version; }
        uint128_t getVersion() const override
            { Mutex mutex( _slaves ); return _version; }
        //@}

        bool isMaster() const override { return true; }
        uint32_t getMasterInstanceID() const override
            { LBDONTCALL; return CO_INSTANCE_INVALID; }

        void addSlave( MasterCMCommand command ) override;
        void removeSlave( NodePtr node, const uint32_t instanceID ) override;
        void removeSlaves( NodePtr node ) override;
        const Nodes getSlaveNodes() const override
            { Mutex mutex( _slaves ); return *_slaves; }

    protected:
        /** The list of subscribed slave nodes. */
        lunchbox::Lockable< Nodes > _slaves;

        /** The current version. */
        uint128_t _version;

        /** Maximum master version allowed to commit. */
        lunchbox::Monitor< uint64_t > _maxVersion;

    private:
        struct SlaveData
        {
            SlaveData() : maxVersion( std::numeric_limits< uint64_t >::max( ))
                        , instanceID( LB_UNDEFINED_UINT32 ) {}
            bool operator == ( const SlaveData& rhs ) const
                { return node == rhs.node && instanceID == rhs.instanceID; }

            NodePtr node;
            uint64_t maxVersion;
            uint32_t instanceID;
        };
        typedef std::vector< SlaveData > SlaveDatas;
        typedef SlaveDatas::const_iterator SlaveDatasCIter;
        typedef SlaveDatas::iterator SlaveDatasIter;

        /** Additional slave data. */
        SlaveDatas _slaveData;

        /** Slave commit queue. */
        DataIStreamQueue _slaveCommits;

        uint128_t _apply( ObjectDataIStream* is );
        void _updateMaxVersion();

        /* The command handlers. */
        bool _cmdSlaveDelta( ICommand& command );
        bool _cmdMaxVersion( ICommand& command );
        bool _cmdDiscard( ICommand& ) { return true; }

        LB_TS_VAR( _cmdThread );
        LB_TS_VAR( _rcvThread );
    };
}

#endif // CO_MASTERCM_H
