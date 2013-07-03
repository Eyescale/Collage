
/* Copyright (c) 2005-2013, Stefan Eilemann <eile@equalizergraphics.com>
 *
 * This file is part of Collage <https://github.com/Eyescale/Collage>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
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

#ifndef CO_NAMEDPIPECONNECTION_H
#define CO_NAMEDPIPECONNECTION_H

#include <co/connection.h> // base class

#include <lunchbox/buffer.h> // member
#include <lunchbox/os.h>
#include <lunchbox/thread.h> // for LB_TS_VAR

#ifndef _WIN32
#  error NamedPipeConnection only supported on Windows
#endif

namespace co
{
    /** An inter-process connection using a named pipe. */
    class NamedPipeConnection : public Connection
    {
    public:
        NamedPipeConnection();

        bool connect() override;
        bool listen() override;
        void acceptNB() override;
        ConnectionPtr acceptSync() override;

        void close() override { _close(); }

        virtual Notifier getNotifier() const override { return _read.hEvent; }

        void readNB( void* buffer, const uint64_t bytes ) override;
        int64_t readSync( void* buffer, const uint64_t bytes,
            const bool ignored ) override;
        int64_t write( const void* buffer,
                               const uint64_t bytes ) override;
    protected:
        virtual ~NamedPipeConnection();

        void _initAIOAccept();
        void _exitAIOAccept();
        void _initAIORead();
        void _exitAIORead();

    private:
        HANDLE _fd;

        // overlapped data structures
        OVERLAPPED _read;
        DWORD      _readDone;
        OVERLAPPED _write;

        LB_TS_VAR( _recvThread );

        bool _connectToNewClient( HANDLE hPipe ) ;
        std::string _getFilename() const;
        bool _createNamedPipe();
        bool _connectNamedPipe();
        void _close();
    };
    typedef lunchbox::RefPtr< NamedPipeConnection > NamedPipeConnectionPtr;
}

#endif //CO_NAMEDPIPECONNECTION_H
