
/* Copyright (c) 2005-2016, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "connection.h"

#include "buffer.h"
#include "connectionDescription.h"
#include "connectionListener.h"
#include "log.h"
#include "pipeConnection.h"
#include "socketConnection.h"
#include "rspConnection.h"

#ifdef _WIN32
#  include "namedPipeConnection.h"
#endif

#include <co/exception.h>

#ifdef COLLAGE_USE_OFED
#  include "rdmaConnection.h"
#endif
#ifdef COLLAGE_USE_UDT
#  include "udtConnection.h"
#endif

#include <lunchbox/scopedMutex.h>
#include <lunchbox/stdExt.h>

#define STATISTICS
namespace co
{
namespace detail
{
class Connection
{
public:
    co::Connection::State state; //!< The connection state
    ConnectionDescriptionPtr description; //!< The connection parameters

    /** The lock used to protect concurrent write calls. */
    mutable lunchbox::Lock sendLock;

    BufferPtr buffer; //!< Current async read buffer
    uint64_t bytes; //!< Current read request size

    /** The listeners on state changes */
    ConnectionListeners listeners;

    uint64_t outBytes; //!< Statistic: written bytes
    uint64_t inBytes; //!< Statistic: read bytes

    Connection()
            : state( co::Connection::STATE_CLOSED )
            , description( new ConnectionDescription )
            , bytes( 0 )
            , outBytes( 0 )
            , inBytes( 0 )
    {
        description->type = CONNECTIONTYPE_NONE;
    }

    ~Connection()
    {
        LBASSERT( state == co::Connection::STATE_CLOSED );
        state = co::Connection::STATE_CLOSED;
        description = 0;

        LBASSERTINFO( !buffer,
                      "Pending read operation during connection destruction" );
    }

    void fireStateChanged( co::Connection* connection )
    {
        for( ConnectionListeners::const_iterator i= listeners.begin();
             i != listeners.end(); ++i )
        {
            (*i)->notifyStateChanged( connection );
        }
    }
};
}

Connection::Connection()
        : _impl( new detail::Connection )
{
    LBVERB << "New Connection @" << (void*)this << std::endl;
}

Connection::~Connection()
{
    LBVERB << "Delete Connection @" << (void*)this << std::endl;
#ifdef STATISTICS
    if( _impl->outBytes || _impl->inBytes )
        LBINFO << *this << ": " << (_impl->outBytes >> 20) << " MB out, "
               << (_impl->inBytes >> 20) << " MB in" << std::endl;
#endif
    delete _impl;
}

bool Connection::operator == ( const Connection& rhs ) const
{
    if( this == &rhs )
        return true;
    if( _impl->description->type != CONNECTIONTYPE_PIPE )
        return false;
    Connection* pipe = const_cast< Connection* >( this );
    return pipe->acceptSync().get() == &rhs;
}

ConnectionPtr Connection::create( ConnectionDescriptionPtr description )
{
    ConnectionPtr connection;
    switch( description->type )
    {
        case CONNECTIONTYPE_TCPIP:
            connection = new SocketConnection;
            break;

        case CONNECTIONTYPE_PIPE:
            connection = new PipeConnection;
            break;

#ifdef _WIN32
        case CONNECTIONTYPE_NAMEDPIPE:
            connection = new NamedPipeConnection;
            break;
#endif

        case CONNECTIONTYPE_RSP:
            connection = new RSPConnection;
            break;

#ifdef COLLAGE_USE_OFED
        case CONNECTIONTYPE_RDMA:
            connection = new RDMAConnection;
            break;
#endif
#ifdef COLLAGE_USE_UDT
        case CONNECTIONTYPE_UDT:
            connection = new UDTConnection;
            break;
#endif

        default:
            LBWARN << "Connection type " << description->type
                   << " not supported" << std::endl;
            return 0;
    }

    if( description->bandwidth == 0 )
        description->bandwidth = connection->getDescription()->bandwidth;

    connection->_setDescription( description );
    return connection;
}

Connection::State Connection::getState() const
{
    return _impl->state;
}

void Connection::_setDescription( ConnectionDescriptionPtr description )
{
    LBASSERT( description.isValid( ));
    LBASSERTINFO( _impl->description->type == description->type,
                  "Wrong connection type in description" );
    _impl->description = description;
    LBASSERT( description->bandwidth > 0 );
}

void Connection::_setState( const State state )
{
    if( _impl->state == state )
        return;
    _impl->state = state;
    _impl->fireStateChanged( this );
}

void Connection::lockSend() const
{
    _impl->sendLock.set();
}

void Connection::unlockSend() const
{
    _impl->sendLock.unset();
}

void Connection::addListener( ConnectionListener* listener )
{
    _impl->listeners.push_back( listener );
}

void Connection::removeListener( ConnectionListener* listener )
{
    ConnectionListeners::iterator i = find( _impl->listeners.begin(),
                                            _impl->listeners.end(), listener );
    if( i != _impl->listeners.end( ))
        _impl->listeners.erase( i );
}

//----------------------------------------------------------------------
// read
//----------------------------------------------------------------------
void Connection::recvNB( BufferPtr buffer, const uint64_t bytes )
{
    LBASSERT( !_impl->buffer );
    LBASSERT( _impl->bytes == 0 );
    LBASSERT( buffer );
    LBASSERT( bytes > 0 );
    LBASSERTINFO( bytes < LB_BIT48,
                  "Out-of-sync network stream: read size " << bytes << "?" );

    _impl->buffer = buffer;
    _impl->bytes = bytes;
    buffer->reserve( buffer->getSize() + bytes );
    readNB( buffer->getData() + buffer->getSize(), bytes );
}

bool Connection::recvSync( BufferPtr& outBuffer, const bool block )
{
    LBASSERTINFO( _impl->buffer,
                  "No pending receive on " << getDescription()->toString( ));

    // reset async IO data
    outBuffer = _impl->buffer;
    const uint64_t bytes = _impl->bytes;
    _impl->buffer = 0;
    _impl->bytes = 0;

    if( _impl->state != STATE_CONNECTED || !outBuffer || bytes == 0 )
        return false;
    LBASSERTINFO( bytes < LB_BIT48,
                  "Out-of-sync network stream: read size " << bytes << "?" );
#ifdef STATISTICS
    _impl->inBytes += bytes;
#endif

    // 'Iterators' for receive loop
    uint8_t* ptr = outBuffer->getData() + outBuffer->getSize();
    uint64_t bytesLeft = bytes;
    int64_t got = readSync( ptr, bytesLeft, block );

    // WAR: fluke notification: On Win32, we get occasionally a data
    // notification and then deadlock when reading from the connection. The
    // callee (Node::handleData) will flag the first read, the underlying
    // SocketConnection will not block and we will restore the AIO operation if
    // no data was present.
    if( got == READ_TIMEOUT )
    {
        _impl->buffer = outBuffer;
        _impl->bytes = bytes;
        outBuffer = 0;
        return true;
    }

    // From here on, receive loop until all data read or error
    while( true )
    {
        if( got < 0 ) // error
        {
            const uint64_t read = bytes - bytesLeft;
            outBuffer->resize( outBuffer->getSize() + read );
            if( bytes == bytesLeft )
                LBDEBUG << "Read on dead connection" << std::endl;
            else
                LBERROR << "Error during read after " << read << " bytes on "
                        << _impl->description << std::endl;
            return false;
        }
        else if( got == 0 )
        {
            // ConnectionSet::select may report data on an 'empty' connection.
            // If we have nothing read so far, we have hit this case.
            if( bytes == bytesLeft )
                return false;
            LBVERB << "Zero bytes read" << std::endl;
        }
        if( bytesLeft > static_cast< uint64_t >( got )) // partial read
        {
            ptr += got;
            bytesLeft -= got;

            readNB( ptr, bytesLeft );
            got = readSync( ptr, bytesLeft, true );
            continue;
        }

        // read done
        LBASSERTINFO( static_cast< uint64_t >( got ) == bytesLeft,
                      got << " != " << bytesLeft );

        outBuffer->resize( outBuffer->getSize() + bytes );
#ifndef NDEBUG
        if( bytes <= 1024 && ( lunchbox::Log::topics & LOG_PACKETS ))
        {
            ptr -= (bytes - bytesLeft); // rewind
            LBINFO << "recv:" << lunchbox::format( ptr, bytes ) << std::endl;
        }
#endif
        return true;
    }

    LBUNREACHABLE;
    return true;
}

BufferPtr Connection::resetRecvData()
{
    BufferPtr buffer = _impl->buffer;
    _impl->buffer = 0;
    _impl->bytes = 0;
    return buffer;
}

//----------------------------------------------------------------------
// write
//----------------------------------------------------------------------
bool Connection::send( const void* buffer, const uint64_t bytes,
                       const bool isLocked )
{
#ifdef STATISTICS
    _impl->outBytes += bytes;
#endif
    LBASSERT( bytes > 0 );
    if( bytes == 0 )
        return true;

    const uint8_t* ptr = static_cast< const uint8_t* >( buffer );

    // possible OPT: We need to lock here to guarantee an atomic transmission of
    // the buffer. Possible improvements are:
    // 1) Disassemble buffer into 'small enough' pieces and use a header to
    //    reassemble correctly on the other side (aka reliable UDP)
    // 2) Introduce a send thread with a thread-safe task queue
    lunchbox::ScopedMutex<> mutex( isLocked ? 0 : &_impl->sendLock );

#ifndef NDEBUG
    if( bytes <= 1024 && ( lunchbox::Log::topics & LOG_PACKETS ))
        LBINFO << "send:" << lunchbox::format( ptr, bytes ) << std::endl;
#endif

    uint64_t bytesLeft = bytes;
    while( bytesLeft )
    {
        try
        {
            const int64_t wrote = this->write( ptr, bytesLeft );
            if( wrote == -1 ) // error
            {
                LBERROR << "Error during write after " << bytes - bytesLeft
                        << " bytes, closing connection" << std::endl;
                close();
                return false;
            }
            else if( wrote == 0 )
                LBINFO << "Zero bytes write" << std::endl;

            bytesLeft -= wrote;
            ptr += wrote;
        }
        catch( const co::Exception& e )
        {
            LBERROR << e.what() << " after " << bytes - bytesLeft
                    << " bytes, closing connection" << std::endl;
            close();
            return false;
        }

    }
    return true;
}

bool Connection::isMulticast() const
{
    return getDescription()->type >= CONNECTIONTYPE_MULTICAST;
}

ConstConnectionDescriptionPtr Connection::getDescription() const
{
    return _impl->description;
}

ConnectionDescriptionPtr Connection::_getDescription()
{
    return _impl->description;
}

std::ostream& operator << ( std::ostream& os, const Connection& connection )
{
    const Connection::State state = connection.getState();
    ConstConnectionDescriptionPtr desc = connection.getDescription();

    os << lunchbox::className( connection ) << " " << (void*)&connection
       << " state " << ( state == Connection::STATE_CLOSED     ? "closed" :
                         state == Connection::STATE_CONNECTING ? "connecting" :
                         state == Connection::STATE_CONNECTED  ? "connected" :
                         state == Connection::STATE_LISTENING  ? "listening" :
                         state == Connection::STATE_CLOSING    ? "closing" :
                         "UNKNOWN" );
    if( desc.isValid( ))
        os << " description " << desc->toString();

    return os;
}
}
