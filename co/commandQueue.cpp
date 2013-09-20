
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

#include "commandQueue.h"

#include "iCommand.h"
#include "exception.h"
#include "node.h"

#include <lunchbox/mtQueue.h>

namespace co
{
namespace detail
{
class CommandQueue
{
public:
    CommandQueue( const size_t maxSize ) : commands( maxSize ) {}

    /** Thread-safe buffer queue. */
    lunchbox::MTQueue< co::ICommand > commands;
};
}

CommandQueue::CommandQueue( const size_t maxSize )
    : _impl( new detail::CommandQueue( maxSize ))
{
}

CommandQueue::~CommandQueue()
{
    flush();
    delete _impl;
}

void CommandQueue::flush()
{
    if( !isEmpty( ))
        LBWARN << "Flushing non-empty command queue" << std::endl;

    _impl->commands.clear();
}

bool CommandQueue::isEmpty() const
{
    return _impl->commands.isEmpty();
}

size_t CommandQueue::getSize() const
{
    return _impl->commands.getSize();
}

void CommandQueue::push( const ICommand& command )
{
    _impl->commands.push( command );
}

void CommandQueue::pushFront( const ICommand& command )
{
    LBASSERT( command.isValid( ));
    _impl->commands.pushFront( command );
}

ICommand CommandQueue::pop( const uint32_t timeout )
{
    LB_TS_THREAD( _thread );

    ICommand command;
    if( !_impl->commands.timedPop( timeout, command ))
        throw Exception( Exception::TIMEOUT_COMMANDQUEUE );

    return command;
}

ICommands CommandQueue::popAll( const uint32_t timeout )
{
    const ICommands& result = _impl->commands.timedPopRange( timeout );

    if( result.empty( ))
        throw Exception( Exception::TIMEOUT_COMMANDQUEUE );
    return result;
}

ICommand CommandQueue::tryPop()
{
    LB_TS_THREAD( _thread );
    ICommand command;
    _impl->commands.tryPop( command );
    return command;
}

}
