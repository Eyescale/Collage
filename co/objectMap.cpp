
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

#include "objectMap.h"

#include "dataIStream.h"
#include "dataOStream.h"
#include "objectFactory.h"

#include <lunchbox/scopedMutex.h>
#include <lunchbox/spinLock.h>

namespace co
{
namespace
{
struct Entry //!< One object map item
{
    Entry() : instance( 0 ), type( OBJECTTYPE_NONE ), own( false ) {}
    Entry( const uint128_t& v, Object* i, const uint32_t t )
        : version( v ), instance( i ), type( t ), own( false ) {}

    uint128_t version;  //!< The current version of the object
    Object* instance;   //!< The object instance, if attached
    uint32_t type;      //!< The object class id
    bool own;           //!< The object is created by us, delete it
};

typedef stde::hash_map< uint128_t, Entry > Map;
typedef Map::iterator MapIter;
typedef Map::const_iterator MapCIter;
typedef std::vector< uint128_t > IDVector;
typedef IDVector::iterator IDVectorIter;
typedef IDVector::const_iterator IDVectorCIter;
}

namespace detail
{
class ObjectMap
{
public:
    ObjectMap( ObjectHandler& h, ObjectFactory& f )
        : handler( h ) , factory( f ) {}

    ~ObjectMap()
    {
        LBASSERTINFO( masters.empty(), "Object map not cleared" );
        LBASSERTINFO( map.empty(), "Object map not cleared" );
    }

    void clear()
    {
        lunchbox::ScopedFastWrite mutex( lock );
        for( ObjectsCIter i = masters.begin(); i != masters.end(); ++i )
        {
            co::Object* object = *i;
            map.erase( object->getID( ));
            handler.deregisterObject( object );
        }
        masters.clear();

        for( MapIter i = map.begin(); i != map.end(); ++i )
            _removeObject( i->second );
        map.clear();
    }

    void _removeObject( Entry& entry )
    {
        if( !entry.instance )
            return;

        handler.unmapObject( entry.instance );
        if( entry.own )
            factory.destroyObject( entry.instance, entry.type );
        entry.instance = 0;
    }

    ObjectHandler& handler;
    ObjectFactory& factory; //!< The 'parent' user

    mutable lunchbox::SpinLock lock;

    Map map; //!< the actual map
    Objects masters; //!< Master objects registered with this instance

    /** Added master objects since the last commit. */
    IDVector added;

    /** Removed master objects since the last commit. */
    IDVector removed;

    /** Changed master objects since the last commit. */
    ObjectVersions changed;
};
}

ObjectMap::ObjectMap( ObjectHandler& handler, ObjectFactory& factory )
        : _impl( new detail::ObjectMap( handler, factory ))
{}

ObjectMap::~ObjectMap()
{
    delete _impl;
}

uint128_t ObjectMap::commit( const uint32_t incarnation )
{
    _commitMasters( incarnation );
    const uint128_t& version = Serializable::commit( incarnation );
    _impl->added.clear();
    _impl->removed.clear();
    _impl->changed.clear();
    return version;
}

bool ObjectMap::isDirty() const
{
    if( Serializable::isDirty( ))
        return true;

    lunchbox::ScopedFastRead mutex( _impl->lock );
    for( ObjectsCIter i =_impl->masters.begin(); i !=_impl->masters.end(); ++i )
        if( (*i)->isDirty( ))
            return true;
    return false;
}

void ObjectMap::_commitMasters( const uint32_t incarnation )
{
    lunchbox::ScopedFastWrite mutex( _impl->lock );

    for( ObjectsCIter i =_impl->masters.begin(); i !=_impl->masters.end(); ++i )
    {
        Object* object = *i;
        if( !object->isDirty() || object->getChangeType() == Object::STATIC )
            continue;

        const ObjectVersion ov( object->getID(), object->commit( incarnation ));
        Entry& entry = _impl->map[ ov.identifier ];
        if( entry.version == ov.version )
            continue;

        entry.version = ov.version;
        _impl->changed.push_back( ov );
    }
    if( !_impl->changed.empty( ))
        setDirty( DIRTY_CHANGED );
}

void ObjectMap::serialize( DataOStream& os, const uint64_t dirtyBits )
{
    Serializable::serialize( os, dirtyBits );
    lunchbox::ScopedFastWrite mutex( _impl->lock );
    if( dirtyBits == DIRTY_ALL )
    {
        for( MapCIter i = _impl->map.begin(); i != _impl->map.end(); ++i )
            os << i->first << i->second.version << i->second.type;
        os << ObjectVersion();
        return;
    }

    if( dirtyBits & DIRTY_ADDED )
    {
        os << _impl->added;
        for( IDVectorCIter i = _impl->added.begin();
             i != _impl->added.end(); ++i )
        {
            const Entry& entry = _impl->map[ *i ];
            os << entry.version << entry.type;
        }
    }
    if( dirtyBits & DIRTY_REMOVED )
        os << _impl->removed;
    if( dirtyBits & DIRTY_CHANGED )
        os << _impl->changed;
}

void ObjectMap::deserialize( DataIStream& is, const uint64_t dirtyBits )
{
    Serializable::deserialize( is, dirtyBits );
    lunchbox::ScopedFastWrite mutex( _impl->lock );
    if( dirtyBits == DIRTY_ALL )
    {
        LBASSERT( _impl->map.empty( ));

        ObjectVersion ov;
        is >> ov;
        while( ov != ObjectVersion( ))
        {
            LBASSERT( _impl->map.find( ov.identifier ) == _impl->map.end( ));
            Entry& entry = _impl->map[ ov.identifier ];
            entry.version = ov.version;
            is >> entry.type >> ov;
        }
        return;
    }

    if( dirtyBits & DIRTY_ADDED )
    {
        IDVector added;
        is >> added;

        for( IDVectorCIter i = added.begin(); i != added.end(); ++i )
        {
            LBASSERT( _impl->map.find( *i ) == _impl->map.end( ));
            Entry& entry = _impl->map[ *i ];
            is >> entry.version >> entry.type;
        }
    }
    if( dirtyBits & DIRTY_REMOVED )
    {
        IDVector removed;
        is >> removed;

        for( IDVectorCIter i = removed.begin(); i != removed.end(); ++i )
        {
            MapIter it = _impl->map.find( *i );
            LBASSERT( it != _impl->map.end( ));
            _impl->_removeObject( it->second );
            _impl->map.erase( it );
        }
    }
    if( dirtyBits & DIRTY_CHANGED )
    {
        ObjectVersions changed;
        is >> changed;

        for( ObjectVersionsCIter i = changed.begin(); i!=changed.end(); ++i)
        {
            const ObjectVersion& ov = *i;
            LBASSERT( _impl->map.find( ov.identifier ) != _impl->map.end( ));

            Entry& entry = _impl->map[ ov.identifier ];
            if( !entry.instance || entry.instance->isMaster( ))
            {
                LBERROR << "Empty or master instance for object "
                        << ov.identifier << " in slave object map" << std::endl;
                continue;
            }

            if( ov.version < entry.instance->getVersion( ))
                LBWARN << "Cannot sync " << entry.instance
                       << " to older version " << ov.version << ", got "
                       << entry.instance->getVersion() << std::endl;
            else
                entry.version = entry.instance->sync( ov.version );
        }
    }
}

void ObjectMap::notifyAttached()
{
    _impl->added.clear();
    _impl->removed.clear();
    _impl->changed.clear();
}

bool ObjectMap::register_( Object* object, const uint32_t type )
{
    LBASSERT( object );
    if( !object )
        return false;

    lunchbox::ScopedFastWrite mutex( _impl->lock );
    MapIter it = _impl->map.find( object->getID( ));
    if( it != _impl->map.end( ))
        return false;

    _impl->handler.registerObject( object );
    const Entry entry( object->getVersion(), object, type );
    _impl->map[ object->getID() ] = entry;
    _impl->masters.push_back( object );
    _impl->added.push_back( object->getID( ));
    setDirty( DIRTY_ADDED );
    return true;
}

bool ObjectMap::deregister( Object* object )
{
    LBASSERT( object );
    if( !object )
        return false;

    lunchbox::ScopedFastWrite mutex( _impl->lock );
    MapIter mapIt = _impl->map.find( object->getID( ));
    ObjectsIter masterIt = std::find( _impl->masters.begin(),
                                      _impl->masters.end(), object );
    if( mapIt == _impl->map.end() || masterIt == _impl->masters.end( ))
        return false;

    _impl->handler.deregisterObject( object );
    _impl->map.erase( mapIt );
    _impl->masters.erase( masterIt );
    _impl->removed.push_back( object->getID( ));
    setDirty( DIRTY_REMOVED );
    return true;
}

Object* ObjectMap::map( const uint128_t& identifier, Object* instance )
{
    if( identifier == 0 )
        return 0;

    lunchbox::ScopedFastWrite mutex( _impl->lock );
    MapIter i = _impl->map.find( identifier );
    LBASSERT( i != _impl->map.end( ));
    if( i == _impl->map.end( ))
    {
        LBWARN << "Object mapping failed, no master registered" << std::endl;
        return 0;
    }

    Entry& entry = i->second;
    if( entry.instance )
    {
        LBASSERTINFO( !instance || entry.instance == instance,
                      entry.instance << " != " << instance )
        if( !instance || entry.instance == instance )
            return entry.instance;

        LBWARN << "Object mapping failed, different instance registered"
               << std::endl;
        return 0;
    }
    LBASSERT( entry.type != OBJECTTYPE_NONE );

    Object* object = instance ? instance :
                                _impl->factory.createObject( entry.type );
    LBASSERT( object );
    if( !object )
        return 0;

    const uint32_t req = _impl->handler.mapObjectNB( object, identifier,
                                                     entry.version,
                                                     getMasterNode( ));
    if( !_impl->handler.mapObjectSync( req ))
    {
        if( !instance )
            _impl->factory.destroyObject( object, entry.type );
        return 0;
    }

    if( object->getVersion() != entry.version )
        LBWARN << "Object " << *object << " could not be mapped to desired "
               << "version, should be " << entry.version << ", but is "
               << object->getVersion() << std::endl;

    entry.instance = object;
    entry.own = !instance;
    return object;
}

bool ObjectMap::unmap( Object* object )
{
    LBASSERT( object );
    if( !object )
        return false;

    lunchbox::ScopedFastWrite mutex( _impl->lock );
    MapIter it = _impl->map.find( object->getID( ));
    if( it == _impl->map.end( ))
        return false;

    _impl->_removeObject( it->second );
    return true;
}

void ObjectMap::clear()
{
    _impl->clear();
}

}
