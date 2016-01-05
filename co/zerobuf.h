
/* Copyright (c) 2015 Daniel.Nachbaur@epfl.ch
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

#ifndef CO_ZEROBUF_OBJECT_H
#define CO_ZEROBUF_OBJECT_H

#include <co/object.h>      // base class
#include <co/dataIStream.h> // used inline
#include <co/dataOStream.h> // used inline

namespace co
{

/**
 * Implements a distributable Collage object for any ZeroBuf object. Clients
 * instantiate this object with a concrete ZeroBuf object.
 */
template< class T >
class ZeroBuf : public T, public Object
{
public:
    /** Construct a new distributable ZeroBuf object. @version 1.4 */
    ZeroBuf() : T(), Object(), _dirty( false ) {}

    /** Copy-construct a distributable ZeroBuf object. @version 1.4 */
    ZeroBuf( const ZeroBuf& rhs ) : T( rhs ), Object( rhs ), _dirty( false ) {}

    virtual ~ZeroBuf() {}

    /** @sa Object::dirty() */
    bool isDirty() const final { return _dirty; }

    /** @sa Object::commit() */
    uint128_t commit( const uint32_t incarnation = CO_COMMIT_NEXT ) final
    {
        const uint128_t& version = co::Object::commit( incarnation );
        _dirty = false;
        return version;
    }

private:
    ChangeType getChangeType() const final { return INSTANCE; }
    void notifyChanging() final { _dirty = true; }

    void getInstanceData( co::DataOStream& os ) final
    {
        os << T::getZerobufSize()
           << Array< const void >( T::getZerobufData(), T::getZerobufSize( ));
    }

    void applyInstanceData( co::DataIStream& is ) final
    {
        size_t size = 0;
        is >> size;
        T::copyZerobufData( is.getRemainingBuffer( size ), size );
    }

    bool _dirty;
};
}

#endif
