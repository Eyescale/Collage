
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder  <cedric.stalder@gmail.com>
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

#ifndef CO_OBJECTINSTANCEDATAOSTREAM_H
#define CO_OBJECTINSTANCEDATAOSTREAM_H

#include "objectDataOStream.h"   // base class

namespace co
{
    /** The DataOStream for object instance data. */
    class ObjectInstanceDataOStream : public ObjectDataOStream
    {
    public:
        ObjectInstanceDataOStream( const ObjectCM* cm );
        virtual ~ObjectInstanceDataOStream();

        void reset() override;

        /** Set up commit of the given version to the receivers. */
        void enableCommit( const uint128_t& version,
                                   const Nodes& receivers ) override;

        /** Set up push of the given version to the receivers. */
        void enablePush( const uint128_t& version, const Nodes& receivers );

        /** Set up sync of the given version to the receivers. */
        void enableSync( const uint128_t& version,
                         const MasterCMCommand& command );

        /** Push a stored instance data. */
        void push( const Nodes& receivers, const uint128_t& objectID,
                   const uint128_t& groupID, const uint128_t& typeID );

        /** Synchronize a stored instance data. */
        void sync( const MasterCMCommand& command );

        /** Set up mapping of the given version to the given node. */
        void enableMap( const uint128_t& version, NodePtr node,
                        const uint32_t instanceID );

        /** Send-on-register instance data to all receivers. */
        void sendInstanceData( const Nodes& receivers );

        /** Send mapping data to the node, using multicast if available. */
        void sendMapData( NodePtr node, const uint32_t instanceID );

    protected:
        void sendData( const void* buffer, const uint64_t size,
                               const bool last ) override;

    private:
        NodeID _nodeID;
        uint32_t _instanceID;
        uint32_t _command;
    };
}
#endif //CO_OBJECTINSTANCEDATAOSTREAM_H
