
/* Copyright (c) 2015-2016 Daniel.Nachbaur@epfl.ch
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

#ifndef CO_DISTRIBUTABLE_H
#define CO_DISTRIBUTABLE_H

#include <co/object.h>      // base class
#include <co/dataIStream.h> // used inline
#include <co/dataOStream.h> // used inline
#include <servus/serializable.h> //used inline

namespace co
{

/**
 * Distributable Collage object for any servus::Serializable object.
 *
 * Clients instantiate this object with a concrete Zerobuf object (or other
 * servus::Serializable) using CRTP. The base class T needs to implement and
 * call an abstract change notification method "virtual void notifyChanging() =
 * 0;" (Zerobuf does this).
 */
template< class T > class Distributable : public T, public Object
{
public:
    /** Construct a new distributable object. @version 1.4 */
    Distributable() : T(), Object(), _dirty( false ) {}

    /** Copy-construct a distributable object. @version 1.4 */
    Distributable( const Distributable& rhs )
        : T( rhs ), Object( rhs ), _dirty( false ) {}

    virtual ~Distributable() {}

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
        const auto& data = T::toBinary();
        os << data.size << Array< const void >( data.ptr.get(), data.size );
    }

    void applyInstanceData( co::DataIStream& is ) final
    {
        size_t size = 0;
        is >> size;
        T::fromBinary( is.getRemainingBuffer( size ), size );
    }

    bool _dirty;
};
}

#endif
