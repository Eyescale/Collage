
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

#ifndef CO_PIPE_CONNECTION_H
#define CO_PIPE_CONNECTION_H

#ifdef _WIN32
#  include <co/namedPipeConnection.h>
#else
#  include "fdConnection.h"
#endif

#include <lunchbox/thread.h>

namespace co
{
    class NamedPipeConnection;
    class PipeConnection;
    typedef lunchbox::RefPtr< PipeConnection > PipeConnectionPtr;
    typedef lunchbox::RefPtr< const PipeConnection > ConstPipeConnectionPtr;

    /**
     * An inter-thread, bi-directional connection using anonymous pipes.
     *
     * The pipe connection is implemented using anonymous pipes, and can
     * therefore only be used between related threads. It consist of a pair of
     * siblings representing the two endpoints.
     */
    class PipeConnection
#ifdef _WIN32
        : public Connection
#else
        : public FDConnection
#endif
    {
    public:
        /** Construct a new pipe connection. */
        CO_API PipeConnection();
        /** Destruct this pipe connection. */
        CO_API virtual ~PipeConnection();

        virtual bool connect() override;
        virtual void close() override { _close(); }

#ifdef _WIN32
        virtual Notifier getNotifier() const override;
#endif

        virtual void acceptNB() override { /* nop */ }

        /** @return the sibling of this pipe connection. */
        virtual ConnectionPtr acceptSync() override { return _sibling; }

    protected:
#ifdef _WIN32
        virtual void readNB( void* buffer, const uint64_t bytes ) override;
        virtual int64_t readSync( void* buffer, const uint64_t bytes,
                                  const bool ignored ) override;
        virtual int64_t write( const void* buffer,
                               const uint64_t bytes ) override;
#endif

    private:
        PipeConnectionPtr _sibling;

#ifdef _WIN32
        NamedPipeConnectionPtr _namedPipe;

        LB_TS_VAR( _recvThread );
#endif
        struct Private;
        Private* _private; // placeholder for binary-compatible changes

        bool _createPipes();
        void _close();
    };
}

#endif //CO_PIPE_CONNECTION_H
