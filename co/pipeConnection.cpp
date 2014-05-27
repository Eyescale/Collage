
/* Copyright (c) 2007-2012, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2011, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "pipeConnection.h"

#include "connectionDescription.h"
#include "node.h"
#ifdef _WIN32
#  include "namedPipeConnection.h"
#endif

#include <lunchbox/log.h>
#include <lunchbox/thread.h>

#include <errno.h>

namespace co
{

PipeConnection::PipeConnection()
{
    ConnectionDescriptionPtr description = _getDescription();
    description->type = CONNECTIONTYPE_PIPE;
    description->bandwidth = 1024000;
}

PipeConnection::~PipeConnection()
{
    _close();
}

//----------------------------------------------------------------------
// connect
//----------------------------------------------------------------------
bool PipeConnection::connect()
{
    LBASSERT( getDescription()->type == CONNECTIONTYPE_PIPE );

    if( !isClosed( ))
        return false;

    _setState( STATE_CONNECTING );
    _sibling = new PipeConnection;
    _sibling->_sibling = this;

    if( !_createPipes( ))
    {
        close();
        return false;
    }

    _setState( STATE_CONNECTED );
    _sibling->_setState( STATE_CONNECTED );
    _connected = true;
    return true;
}

#ifdef _WIN32

Connection::Notifier PipeConnection::getNotifier() const
{
    if( !_namedPipe )
        return 0;
    return _namedPipe->getNotifier();
}

bool PipeConnection::_createPipes()
{
    std::stringstream pipeName;
    pipeName << "\\\\.\\pipe\\Collage." << UUID( true );

    ConnectionDescriptionPtr desc = new ConnectionDescription;
    desc->type = CONNECTIONTYPE_NAMEDPIPE;
    desc->setFilename( pipeName.str( ));

    ConnectionPtr connection = Connection::create( desc );
    _namedPipe = static_cast< NamedPipeConnection* >( connection.get( ));
    if( !_namedPipe->listen( ))
        return false;
    _namedPipe->acceptNB();

    connection = Connection::create( desc );
    _sibling->_namedPipe = static_cast<NamedPipeConnection*>(connection.get());
    if( !_sibling->_namedPipe->connect( ))
    {
        _sibling->_namedPipe = 0;
        return false;
    }

    connection = _namedPipe->acceptSync();
    _namedPipe = static_cast< NamedPipeConnection* >(connection.get( ));
    return true;
}

void PipeConnection::_close()
{
    if( isClosed( ))
        return;

    _connected = false;
    _namedPipe->close();
    _namedPipe = 0;
    _sibling = 0;

    _setState( STATE_CLOSED );
}

void PipeConnection::readNB( void* buffer, const uint64_t bytes )
{
    if( isClosed( ))
        return;
    _namedPipe->readNB( buffer, bytes );
}

int64_t PipeConnection::readSync( void* buffer, const uint64_t bytes,
                                       const bool ignored )
{
    if( isClosed( ))
        return -1;

    const int64_t bytesRead = _namedPipe->readSync( buffer, bytes, ignored );

    if( bytesRead == -1 )
        close();

    return bytesRead;
}

int64_t PipeConnection::write( const void* buffer, const uint64_t bytes )
{
    if( !isConnected( ))
        return -1;

    return _namedPipe->write( buffer, bytes );
}

#else // !_WIN32

bool PipeConnection::_createPipes()
{
    int pipeFDs[2];
    if( ::pipe( pipeFDs ) == -1 )
    {
        LBERROR << "Could not create pipe: " << strerror( errno );
        close();
        return false;
    }

    _readFD  = pipeFDs[0];
    _sibling->_writeFD = pipeFDs[1];

    if( ::pipe( pipeFDs ) == -1 )
    {
        LBERROR << "Could not create pipe: " << strerror( errno );
        close();
        return false;
    }

    _sibling->_readFD  = pipeFDs[0];
    _writeFD = pipeFDs[1];
    return true;
}

void PipeConnection::_close()
{
    if( isClosed( ))
        return;

    if( _writeFD > 0 )
    {
        ::close( _writeFD );
        _writeFD = 0;
    }
    if( _readFD > 0 )
    {
        ::close( _readFD );
        _readFD  = 0;
    }

    _connected = false;
    _setState( STATE_CLOSED );
    _sibling = 0;
}
#endif // else _WIN32

ConnectionPtr PipeConnection::acceptSync()
{
    _connected.waitEQ( true );
    return _sibling;
}

}
