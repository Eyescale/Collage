
/* Copyright (c) 2007-2017, Stefan Eilemann <eile@equalizergraphics.com>
 *                          Cedric Stalder  <cedric.stalder@gmail.com>
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

#include "objectSlaveDataOStream.h"

#include "log.h"
#include "object.h"
#include "objectCommand.h"
#include "objectDataIStream.h"
#include "objectDataOCommand.h"
#include "versionedMasterCM.h"

namespace co
{
ObjectSlaveDataOStream::ObjectSlaveDataOStream(const ObjectCM* cm)
    : ObjectDataOStream(cm)
    , _commit(servus::make_UUID())
{
}

ObjectSlaveDataOStream::~ObjectSlaveDataOStream()
{
}

void ObjectSlaveDataOStream::enableSlaveCommit(NodePtr node)
{
    _version = servus::make_UUID();
    _setupConnection(node, false /* useMulticast */);
    _enable();
}

void ObjectSlaveDataOStream::sendData(const void* data, const uint64_t size,
                                      const bool last)
{
    send(CMD_OBJECT_SLAVE_DELTA, COMMANDTYPE_OBJECT,
         _cm->getObject()->getMasterInstanceID(), data, size, last)
        << _commit;

    if (last)
        _commit = servus::make_UUID();
}
}
