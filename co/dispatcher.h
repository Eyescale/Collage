
/* Copyright (c) 2006-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_DISPATCHER_H
#define CO_DISPATCHER_H

#include <co/api.h>
#include <co/commandFunc.h> // used inline
#include <co/types.h>

namespace co
{
namespace detail { class Dispatcher; }

    /**
     * Provided command dispatch functionality to networked objects.
     *
     * Command dispatch in performed through a command queue and command handler
     * table.
     */
    class Dispatcher
    {
    public:
        /** The signature of the base Dispatcher callback. @version 1.0 */
        typedef CommandFunc< Dispatcher > Func;

        /** @internal NOP assignment operator. */
        const Dispatcher& operator = ( const Dispatcher& ) { return *this; }

        /**
         * Register a command member function for a command.
         *
         * If the destination queue is 0, the command function is invoked
         * directly upon dispatch, otherwise it is pushed to the given queue and
         * invoked during the processing of the command queue.
         *
         * @param command the command.
         * @param func the functor to handle the command.
         * @param queue the queue to which the the command is dispatched
         * @version 1.0
         */
        template< typename T > void
        registerCommand( const uint32_t command, const CommandFunc< T >& func,
                         CommandQueue* queue );

        /**
         * Dispatch a command from the receiver thread to the registered queue.
         *
         * @param command the command.
         * @return true if the command was dispatched, false if not.
         * @sa registerCommand
         * @version 1.0
         */
        CO_API virtual bool dispatchCommand( ICommand& command );

    protected:
        CO_API Dispatcher();
        CO_API Dispatcher( const Dispatcher& from );
        CO_API virtual ~Dispatcher();

        /**
         * The default handler for handling commands.
         *
         * @param command the command
         * @return false
         */
        CO_API bool _cmdUnknown( ICommand& command );

    private:
        detail::Dispatcher* const _impl;

        CO_API void _registerCommand( const uint32_t command,
                                      const Func& func, CommandQueue* queue );
    };

    template< typename T >
    void Dispatcher::registerCommand( const uint32_t command,
                                      const CommandFunc< T >& func,
                                      CommandQueue* queue )
    {
        _registerCommand( command, Dispatcher::Func( func ), queue );
    }
}
#endif // CO_DISPATCHER_H
