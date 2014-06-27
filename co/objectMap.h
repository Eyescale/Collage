
/* Copyright (c) 2012-2013, Daniel Nachbaur <danielnachbaur@googlemail.com>
 *               2012-2014, Stefan Eilemann <eile@eyescale.ch>
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

#ifndef CO_OBJECTMAP_H
#define CO_OBJECTMAP_H

#include <co/serializable.h>      // base class

namespace co
{
namespace detail { class ObjectMap; }

/**
 * A distributed object registry.
 *
 * The object map takes care of distribution and synchronization of registered
 * objects across all slave instances. Objects are registered with an
 * additional type to resolve the creation of new objects during mapping.
 * This creation is handled by an ObjectFactory which has to be provided and
 * implemented for the desired object types.
 */
class ObjectMap : public Serializable
{
public:
    /**
     * Construct a new object map.
     *
     * @param handler used for object registration and mapping
     * @param factory to create & destroy slave objects
     * @version 1.0
     */
    CO_API ObjectMap( ObjectHandler& handler, ObjectFactory& factory );

    /**
     * Destroy this object map.
     *
     * All registered and mapped objects will be deregistered and unmapped.
     * All mapped and owned objects will be destroyed using the object factory.
     * @version 1.0
     */
    CO_API virtual ~ObjectMap();

    /**
     * Add and register a new object as master instance to this object map.
     *
     * Upon registering using the map's object handler, this object will be
     * remembered for serialization on the next commit of this object map.
     *
     * @param object the new object to add and register
     * @param type unique object type to create object via slave factory
     * @return false on failed ObjectHandler::registerObject, true otherwise
     * @version 1.0
     */
    CO_API bool register_( Object* object, const uint32_t type );

    /**
     * Remove and deregister an object from this object map.
     *
     * Upon deregistering using the map's object handler, this object will be
     * remembered for unmap and possible deletion on the next commit of this
     * object map.
     *
     * @param object the object to remove and deregister
     * @return false on if object was not registered, true otherwise
     * @version 1.0
     */
    CO_API bool deregister( Object* object );

    /**
     * Map and return an object.
     *
     * The object is either created via its type specified upon registering or
     * an already created instance is used if passed to this function. Passed
     * instances will not be considered for deletion during explicit unmap(),
     * implicit unmap caused by deregister(), or destruction of this object map.
     *
     * The object will be mapped to the version that was current on registration
     * time.
     *
     * @param identifier unique object identifier used for map operation
     * @param instance already created instance to skip factory creation
     * @return 0 if not registered, the valid instance otherwise
     * @version 1.0
     */
    CO_API Object* map( const uint128_t& identifier, Object* instance = 0 );

    /**
     * Unmap an object from the object map.
     *
     * The object is unmapped using the map's object handler and will not be
     * considered for further synchronization. The object will be destructed if
     * if was created by the object map.
     *
     * @param object the object to unmap
     * @return false on if object was not mapped, true otherwise
     * @version 1.0
     */
    CO_API bool unmap( Object* object );

    /** Deregister or unmap all registered and mapped objects. @version 1.0 */
    CO_API void clear();

    /** Commit all registered objects. @version 1.0 */
    CO_API uint128_t commit( const uint32_t incarnation =
                                     CO_COMMIT_NEXT ) override;

protected:
    CO_API bool isDirty() const override; //!< @internal

    /** @internal */
    CO_API void serialize( DataOStream& os,
                                   const uint64_t dirtyBits ) override;

    /** @internal */
    CO_API void deserialize( DataIStream& is,
                                     const uint64_t dirtyBits ) override;
    /** @internal */
    ChangeType getChangeType() const override { return DELTA; }
    CO_API void notifyAttached() override; //!< @internal

    /** @internal The changed parts of the object since the last serialize(). */
    enum DirtyBits
    {
        DIRTY_ADDED       = Serializable::DIRTY_CUSTOM << 0, // 1
        DIRTY_REMOVED     = Serializable::DIRTY_CUSTOM << 1, // 2
        DIRTY_CHANGED     = Serializable::DIRTY_CUSTOM << 2, // 4
        DIRTY_CUSTOM      = Serializable::DIRTY_CUSTOM << 3  // 8
    };

private:
    detail::ObjectMap* const _impl;

    /** @internal Commit and note new master versions. */
    void _commitMasters( const uint32_t incarnation );
};
}
#endif // CO_OBJECTMAP_H
