
/* Copyright (c) 2010-2012, Stefan Eilemann <eile@eyescale.ch>
 *                    2010, Cedric Stalder  <cedric.stalder@gmail.com>
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

#include "objectDataOStream.h"

#include "log.h"
#include "objectCM.h"
#include "objectDataOCommand.h"

#include <co/plugins/compressorTypes.h>

namespace co
{
ObjectDataOStream::ObjectDataOStream( const ObjectCM* cm )
        : _cm( cm )
        , _version( VERSION_INVALID )
        , _sequence( 0 )
{
    const Object* object = cm->getObject();
    const uint32_t name = object->chooseCompressor();
    _initCompressor( name );
    LBLOG( LOG_OBJECTS )
        << "Using byte compressor 0x" << std::hex << name << std::dec << " for "
        << lunchbox::className( object ) << std::endl;
}

void ObjectDataOStream::reset()
{
    DataOStream::reset();
    _sequence = 0;
    _version = VERSION_INVALID;
}

void ObjectDataOStream::enableCommit( const uint128_t& version,
                                      const Nodes& receivers )
{
    _version = version;
    _setupConnections( receivers );
    _enable();
}

ObjectDataOCommand ObjectDataOStream::send(
    const uint32_t cmd, const uint32_t type, const uint32_t instanceID,
    const uint64_t size, const bool last )
{
    LBASSERT( _version != VERSION_INVALID );
    const uint32_t sequence = _sequence++;
    if( last )
        _sequence = 0;

    return ObjectDataOCommand( getConnections(), cmd, type,
                               _cm->getObject()->getID(), instanceID, _version,
                               sequence, size, last, this );
}

}
