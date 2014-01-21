
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_VERSIONEDSLAVECM_H
#define CO_VERSIONEDSLAVECM_H

#include "objectCM.h"            // base class
#include "objectDataIStream.h"      // member
#include "objectSlaveDataOStream.h" // member

#include <lunchbox/mtQueue.h>     // member
#include <lunchbox/pool.h>        // member
#include <lunchbox/thread.h>      // thread-safety macro

namespace co
{
    class Node;

    /**
     * An object change manager handling changes for versioned slave instances.
     */
    class VersionedSlaveCM : public ObjectCM
    {
    public:
        VersionedSlaveCM( Object* object, uint32_t masterInstanceID );
        virtual ~VersionedSlaveCM();

        void init() override {}

        /**
         * @name Versioning
         */
        //@{
        uint128_t commit( const uint32_t incarnation ) override;
        uint128_t sync( const uint128_t& version ) override;

        uint128_t getHeadVersion() const override;
        uint128_t getVersion() const override { return _version; }
        //@}

        bool isMaster() const override { return false; }

        uint32_t getMasterInstanceID() const override
            { return _masterInstanceID; }
        void setMasterNode( NodePtr node ) override { _master = node; }
        NodePtr getMasterNode() override { return _master; }

        bool addSlave( const MasterCMCommand& ) override
            { LBDONTCALL; return false; }
        void removeSlaves( NodePtr ) override {}

        void applyMapData( const uint128_t& version ) override;
        void addInstanceDatas( const ObjectDataIStreamDeque&,
                               const uint128_t& startVersion ) override;
    private:
        /** The current version. */
        uint128_t _version;

        /** istream for receiving the current version */
        ObjectDataIStream* _currentIStream;

        /** The change queue. */
        lunchbox::MTQueue< ObjectDataIStream* > _queuedVersions;

        /** Cached input streams (+decompressor) */
        lunchbox::Pool< ObjectDataIStream, true > _iStreamCache;

        /** The output stream for slave object commits. */
        ObjectSlaveDataOStream _ostream;

        /** The node holding the master object. */
        NodePtr _master;

        /** The instance identifier of the master object. */
        uint32_t _masterInstanceID;

        void _syncToHead();
        void _releaseStream( ObjectDataIStream* stream );
        void _sendAck();

        /** Apply the data in the input stream to the object */
        void _unpackOneVersion( ObjectDataIStream* is );

        /* The command handlers. */
        bool _cmdData( ICommand& command );

        LB_TS_VAR( _cmdThread );
        LB_TS_VAR( _rcvThread );
    };
}

#endif // CO_VERSIONEDSLAVECM_H
