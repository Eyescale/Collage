
/* Copyright (c) 2011-2012, Stefan Eilemann <eile@eyescale.ch>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "worker.h"

#include "iCommand.h"

namespace co
{
template< class Q > void WorkerThread< Q >::run()
{
    while( !stopRunning( ))
    {
        while( _commands.isEmpty( ))
            if( !notifyIdle( )) // nothing to do
                break;

        const ICommands& commands = _commands.popAll();
        LBASSERT( !commands.empty( ));

        for( ICommandsCIter i = commands.begin(); i != commands.end(); ++i )
        {
            ICommand& command = const_cast< ICommand& >( *i );
            if( !command( ))
            {
                LBABORT( "Error handling " << command );
            }
            if( stopRunning( ))
                break;

            _commands.pump();
        }
    }

    _commands.flush();
    LBINFO << "Leaving worker thread " << lunchbox::className( this )
           << std::endl;
}

}
