
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "queueItem.h"

#include "queueMaster.h"


namespace co
{
namespace detail
{
class QueueItem
{
public:
    explicit QueueItem( co::QueueMaster& queueMaster_ )
        : queueMaster( queueMaster_ )
    {}

    QueueItem( const QueueItem& rhs )
        : queueMaster( rhs.queueMaster )
    {}

    co::QueueMaster& queueMaster;
};
}

QueueItem::QueueItem( QueueMaster& master )
    : DataOStream()
    , _impl( new detail::QueueItem( master ))
{
    enableSave();
    _enable();
}

QueueItem::QueueItem( const QueueItem& rhs )
    : DataOStream()
    , _impl( new detail::QueueItem( *rhs._impl ))
{
    enableSave();
    _enable();
}

QueueItem::~QueueItem()
{
    _impl->queueMaster._addItem( *this );
    disable();
    delete _impl;
}

}
