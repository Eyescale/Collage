
/* Copyright (c) 2011-2014, Stefan Eilemann <eile@eyescale.ch>
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

#ifndef CO_WORKER_H
#define CO_WORKER_H

#include <co/api.h>          // CO_API definition
#include <co/types.h>
#include <lunchbox/thread.h>  // base class
#include <limits.h>

#ifndef CO_WORKER_API
#  define CO_WORKER_API CO_API
#endif

namespace co
{
/** A worker thread processing items out of a CommandQueue. */
template< class Q > class WorkerThread : public lunchbox::Thread
{
public:
    /** Construct a new worker thread. @version 1.0 */
    WorkerThread( const size_t maxSize = ULONG_MAX ) : _commands( maxSize ) {}

    /** Destruct the worker. @version 1.0 */
    virtual ~WorkerThread() {}

    /** @return the queue to the worker thread. @version 1.0 */
    Q* getWorkerQueue() { return &_commands; }

protected:
    /** @sa lunchbox::Thread::run() */
    CO_WORKER_API void run() override;

    /** @return true to stop the worker thread. @version 1.0 */
    virtual bool stopRunning() { return false; }

    /** @return true to indicate pending idle tasks. @version 1.0 */
    virtual bool notifyIdle() { return false; }

private:
    /** The receiver->worker thread command queue. */
    Q _commands;
};

typedef WorkerThread< CommandQueue > Worker; // instantiated in worker.cpp
}
#endif //CO_WORKER_H
