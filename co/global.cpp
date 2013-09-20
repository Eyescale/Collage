
/* Copyright (c) 2005-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "global.h"

#include <lunchbox/pluginRegistry.h>

#include <limits>
#include <stdlib.h>

#include <vector>
#include <sstream>

namespace co
{
#define SEPARATOR '#'

#ifndef Darwin
#  define BIG_SEND
#endif

namespace
{

static uint32_t _getObjectBufferSize()
{
    const char* env = getenv( "CO_OBJECT_BUFFER_SIZE" );
    if( !env )
        return 60000;

    const int64_t size = atoi( env );
    if( size > 0 )
        return size;

    return 60000;
}

static int32_t _getTimeout()
{
    const char* env = getenv( "CO_TIMEOUT" );
    if( !env )
        return 300000; // == 5min

    const int32_t size = atoi( env );
    if( size <= 0 )
        return 300000; // == 5min

    return size;
}

uint16_t    _defaultPort = 0;
uint32_t    _objectBufferSize = _getObjectBufferSize();
int32_t     _iAttributes[Global::IATTR_ALL] =
{
    100,   // INSTANCE_CACHE_SIZE
    100,   // NODE_SEND_QUEUE_SIZE
    100,   // NODE_SEND_QUEUE_AGE
    10,    // RSP_TIMEOUT
    1,     // RSP_ERROR_DOWNSCALE
    5,     // RSP_ERROR_UPSCALE
    20,    // RSP_ERROR_MAXSCALE
    3,     // RSP_MIN_SENDRATE_SHIFT
#ifdef BIG_SEND
    64,    // RSP_NUM_BUFFERS
    5,     // RSP_ACK_FREQUENCY
    65000, // UDP_MTU
#else
    1024,  // RSP_NUM_BUFFERS
    17,    // RSP_ACK_FREQUENCY
    1470,  // UDP_MTU
#endif
    524288, // UDP_BUFFER_SIZE
    1,      // QUEUE_MIN_SIZE
    1,      // QUEUE_REFILL
    8,      // RDMA_RING_BUFFER_SIZE_MB
    512,    // RDMA_SEND_QUEUE_DEPTH
    5000,   // RDMA_RESOLVE_TIMEOUT_MS
    1,      // IATTR_ROBUSTNESS
    _getTimeout(), // IATTR_TIMEOUT_DEFAULT
    1023,   // IATTR_OBJECT_COMPRESSION
    0,      // IATTR_CMD_QUEUE_LIMIT
};
}

bool Global::fromString(const std::string& data )
{
    if( data.empty() || data[0] != SEPARATOR )
        return false;

    std::vector< uint32_t > newGlobals;
    newGlobals.reserve( IATTR_ALL );

    size_t endMarker( 1u );
    while( true )
    {
        const size_t startMarker = data.find( SEPARATOR, endMarker );
        if( startMarker == std::string::npos )
            break;

        endMarker = data.find( SEPARATOR, startMarker + 1 );
        if( endMarker == std::string::npos )
            break;

        const std::string sub = data.substr( startMarker + 1,
                                             endMarker - startMarker - 1 );
        if( !sub.empty() && (isdigit( sub[0] ) || sub[0] == '-') )
            newGlobals.push_back( atoi( sub.c_str( )) );
        else
            break;
    }

    // only apply a 'complete' global list
    if( newGlobals.size() != IATTR_ALL )
    {
        LBWARN << "Expected " << unsigned( IATTR_ALL ) << " globals, got "
               << newGlobals.size() << std::endl;
        return false;
    }

    std::copy( newGlobals.begin(), newGlobals.end(), _iAttributes );
    return true;
}

void Global::toString( std::string& data )
{
    std::stringstream stream;
    stream << SEPARATOR << SEPARATOR;

    for( uint32_t i = 0; i < IATTR_ALL; ++i )
        stream << _iAttributes[i] << SEPARATOR;

    stream << SEPARATOR;
    data = stream.str();
}

void Global::setDefaultPort( const uint16_t port )
{
    _defaultPort = port;
}

uint16_t Global::getDefaultPort()
{
    return _defaultPort;
}

void Global::setObjectBufferSize( const uint32_t size )
{
    _objectBufferSize = size;
}

uint32_t Global::getObjectBufferSize()
{
    return  _objectBufferSize;
}

lunchbox::PluginRegistry& Global::getPluginRegistry()
{
    static lunchbox::PluginRegistry pluginRegistry;
    return pluginRegistry;
}

void Global::setIAttribute( const IAttribute attr, const int32_t value )
{
    _iAttributes[ attr ] = value;
}

int32_t Global::getIAttribute( const IAttribute attr )
{
    return _iAttributes[ attr ];
}

uint32_t Global::getTimeout()
{
    return getIAttribute( IATTR_ROBUSTNESS ) ?
        getIAttribute( IATTR_TIMEOUT_DEFAULT ) : LB_TIMEOUT_INDEFINITE;
}

uint32_t Global::getKeepaliveTimeout()
{
    const char* env = getenv( "CO_KEEPALIVE_TIMEOUT" );
    if( !env )
        return 2000; // ms

    const int64_t size = atoi( env );
    if( size == 0 )
        return 2000; // ms

    return size;
}

size_t Global::getCommandQueueLimit()
{
    const int32_t limit = getIAttribute( IATTR_CMD_QUEUE_LIMIT );
    if( limit > 0 )
        return size_t( limit ) << 10;
    return std::numeric_limits< size_t >::max();
}

}
