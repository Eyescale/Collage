
/* Copyright (c) 2011-2012, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "dataIStreamQueue.h"

#include "objectDataICommand.h"
#include "objectDataIStream.h"

namespace co
{
DataIStreamQueue::DataIStreamQueue()
{}

DataIStreamQueue::~DataIStreamQueue()
{
    LBASSERTINFO( _pending.empty(), "Incomplete commits pending" );
    LBASSERTINFO( _queued.isEmpty(), _queued.getSize() << " unapplied commits" )

    for( PendingStreamsCIter i = _pending.begin(); i != _pending.end(); ++i )
        delete i->second;
    _pending.clear();

    QueuedStream stream;
    while( _queued.tryPop( stream ))
        delete stream.second;
}

ObjectDataIStream* DataIStreamQueue::tryPop()
{
    QueuedStream stream( uint128_t( 0 ), (ObjectDataIStream*)0 );
    _queued.tryPop( stream );
    return stream.second;
}

ObjectDataIStream* DataIStreamQueue::pull( const uint128_t& key )
{
    ObjectDataIStream* is = 0;
    QueuedStreams unusedStreams;
    while( !is )
    {
        QueuedStream candidate = _queued.pop();
        if( candidate.first == key )
            is = candidate.second;
        else
            unusedStreams.push_back( candidate );
    }

    _queued.pushFront( unusedStreams );
    return is;
}

void DataIStreamQueue::recycle( ObjectDataIStream* stream )
{
#ifdef CO_AGGRESSIVE_CACHING
    stream->reset();
    _iStreamCache.release( stream );
#else
    delete stream;
#endif
}

bool DataIStreamQueue::addDataCommand( const uint128_t& key, ICommand& command )
{
    LB_TS_THREAD( _thread );
    LBASSERTINFO( _pending.size() < 100, "More than 100 pending commits");

    ObjectDataIStream* istream = 0;
    PendingStreams::iterator i = _pending.find( key );
    if( i == _pending.end( ))
        istream = _iStreamCache.alloc();
    else
        istream = i->second;

    istream->addDataCommand( command );
    if( istream->isReady( ))
    {
        if( i != _pending.end( ))
            _pending.erase( i );

        _queued.push( QueuedStream( key, istream ));
        //LBLOG( LOG_OBJECTS ) << "Queued commit " << key << std::endl;
        return true;
    }

    if( i == _pending.end( ))
    {
        _pending[ key ] = istream;
        //LBLOG( LOG_OBJECTS ) << "New incomplete commit " << key << std::endl;
        return false;
    }

    //LBLOG(LOG_OBJECTS) << "Add data to incomplete commit " << key <<std::endl;
    return false;
}

}
