
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

#ifndef CO_COMMANDQUEUE_H
#define CO_COMMANDQUEUE_H

#include <co/api.h>
#include <co/types.h>
#include <lunchbox/thread.h>

namespace co
{
namespace detail { class CommandQueue; }

    /** A thread-safe, blocking queue for ICommand buffers. */
    class CommandQueue : public lunchbox::NonCopyable
    {
    public:
        /**
         * Construct a new command queue.
         *
         * @param maxSize the maximum number of enqueued commands.
         * @version 1.0
        */
        CO_API CommandQueue( const size_t maxSize = ULONG_MAX );

        /** Destruct a new command queue. @version 1.0 */
        CO_API virtual ~CommandQueue();

        /**
         * Push a command to the queue.
         *
         * @param command the command.
         * @version 1.0
         */
        CO_API virtual void push( const ICommand& command );

        /** Push a command to the front of the queue. @version 1.0 */
        CO_API virtual void pushFront( const ICommand& command );

        /**
         * Pop a command from the queue.
         *
         * @param timeout the time in ms to wait for the operation.
         * @return the next command in the queue.
         * @throw Exception on timeout.
         * @version 1.0
         */
        CO_API virtual ICommand pop( const uint32_t timeout =
                                         LB_TIMEOUT_INDEFINITE );

        /**
         * Pop all, but at least one command from the queue.
         *
         * @param timeout the time in ms to wait for the operation.
         * @return one or more commands in the queue.
         * @throw Exception on timeout.
         * @version 1.0
         */
        CO_API virtual ICommands popAll( const uint32_t timeout =
                                             LB_TIMEOUT_INDEFINITE );

        /**
         * Try to pop a command from the queue.
         *
         * @return the next command in the queue, or 0 if no command is queued.
         * @version 1.0
         */
        CO_API virtual ICommand tryPop();

        /**
         * @return <code>true</code> if the command queue is empty,
         *         <code>false</code> if not.
         * @version 1.0
         */
        CO_API bool isEmpty() const;

        /** Flush all pending commands. @version 1.0 */
        CO_API void flush();

        /** @return the size of the queue. @version 1.0 */
        CO_API size_t getSize() const;

        /** @internal trigger internal processing (message pump) */
        virtual void pump() {};

        LB_TS_VAR( _thread );

    private:
        detail::CommandQueue* const _impl;
    };
}

#endif //CO_COMMANDQUEUE_H
