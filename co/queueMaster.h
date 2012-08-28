
/* Copyright (c) 2011, Stefan Eilemann <eile@eyescale.ch> 
 *               2011, Carsten Rohn <carsten.rohn@rtt.ag> 
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

#ifndef CO_QUEUEMASTER_H
#define CO_QUEUEMASTER_H

#include <co/object.h> // base class


namespace co
{
class QueueItem;
namespace detail { class QueueMaster; }

/**
 * The producer end of a distributed queue.
 *
 * One instance of this class is registered with a LocalNode to for the producer
 * end of a distributed queue. One or more QueueSlave instances are mapped to
 * this master instance and consume the data.
 */
class QueueMaster : public Object
{
public:
    /** Construct a new queue master. @version 1.1.6 */
    CO_API QueueMaster();

    /** Destruct this queue master. @version 1.1.6 */
    virtual CO_API ~QueueMaster();

    /**
     * Enqueue a new queue item.
     *
     * The returned queue item can be enhanced with additional using the stream
     * operators provided by DataOStream. Note that the item is enqueued if the
     * returned item object is destroyed, i.e. if it runs out of scope.
     *
     * @return the item to enqueue.
     * @version 1.0
     */
    CO_API QueueItem push();

    /** Remove all enqueued items. @version 1.1.6 */
    CO_API void clear();

private:
    detail::QueueMaster* const _impl;

    CO_API virtual void attach( const UUID& id, const uint32_t instanceID );

    virtual ChangeType getChangeType() const { return STATIC; }
    virtual void getInstanceData( co::DataOStream& os );
    virtual void applyInstanceData( co::DataIStream& ) { LBDONTCALL }

    friend class QueueItem;
    void _addItem( QueueItem& item );
};

} // co

#endif // CO_QUEUEMASTER_H
