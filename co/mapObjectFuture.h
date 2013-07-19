
/* Copyright (c) 2013, Stefan.Eilemann@epfl.ch
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

#ifndef CO_MAPOBJECTFUTURE_H
#define CO_MAPOBJECTFUTURE_H

#include "objectStore.h"

namespace co
{
namespace
{

class MapObjectFuture : public lunchbox::FutureImpl< bool >
{
public:
    MapObjectFuture( ObjectStore* store, const uint32_t request )
        : store_( store ), request_( request ), state_( PENDING )
    {}
    virtual ~MapObjectFuture() { LBASSERT( wait( )); }

    virtual bool wait() final
    {
        if( state_ == PENDING )
            state_ = store_->mapObjectSync( request_ ) ? TRUE : FALSE;
        return state_ == TRUE;
    }

private:
    ObjectStore* const store_;
    const uint32_t request_;
    enum
    {
        PENDING,
        TRUE,
        FALSE
    }
    state_;
};

}
}
#endif
