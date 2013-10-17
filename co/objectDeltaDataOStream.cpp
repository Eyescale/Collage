
/* Copyright (c) 2007-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "objectDeltaDataOStream.h"

#include "object.h"
#include "objectICommand.h"
#include "objectCM.h"
#include "objectDataOCommand.h"

namespace co
{
ObjectDeltaDataOStream::ObjectDeltaDataOStream( const ObjectCM* cm )
        : ObjectDataOStream( cm )
{}

ObjectDeltaDataOStream::~ObjectDeltaDataOStream()
{}

void ObjectDeltaDataOStream::sendData( const void*, const uint64_t size,
                                       const bool last )
{
    ObjectDataOStream::send( CMD_OBJECT_DELTA, COMMANDTYPE_OBJECT,
                             CO_INSTANCE_ALL, size, last );
}

}
