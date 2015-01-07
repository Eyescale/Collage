
/* Copyright (c) 2005-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *               2011-2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "object.h"

#include "dataIStream.h"
#include "dataOStream.h"
#include "deltaMasterCM.h"
#include "fullMasterCM.h"
#include "global.h"
#include "log.h"
#include "nodeCommand.h"
#include "nullCM.h"
#include "objectCM.h"
#include "objectOCommand.h"
#include "staticMasterCM.h"
#include "staticSlaveCM.h"
#include "types.h"
#include "unbufferedMasterCM.h"
#include "versionedSlaveCM.h"

#include <lunchbox/scopedMutex.h>
#include <pression/compressor.h>
#include <pression/plugins/compressor.h>
#include <iostream>

namespace co
{
namespace detail
{
class Object
{
public:
    Object()
        : id( lunchbox::make_UUID( ))
        , instanceID( CO_INSTANCE_INVALID )
        , cm( ObjectCM::ZERO )
        {}

    /** The session-unique object identifier. */
    uint128_t id;

    /** The node where this object is attached. */
    LocalNodePtr localNode;

    /** A session-unique identifier of the concrete instance. */
    uint32_t instanceID;

    /** The object's change manager. */
    ObjectCMPtr cm;
};
}

Object::Object()
    : impl_( new detail::Object )
{}

Object::Object( const Object& object )
    : Dispatcher( object )
    , impl_( new detail::Object )
{}

Object::~Object()
{
    LBASSERTINFO( !isAttached(),
                  "Object " << impl_->id << " is still attached to node " <<
                  impl_->localNode->getNodeID());

    if( impl_->localNode )
        impl_->localNode->releaseObject( this );

    impl_->localNode = 0;
    impl_->cm = 0;
    delete impl_;
}

bool Object::isAttached() const
{
    return impl_->instanceID != CO_INSTANCE_INVALID;
}

LocalNodePtr Object::getLocalNode()
{
    return impl_->localNode;
}

void Object::setID( const uint128_t& identifier )
{
    LBASSERT( !isAttached( ));
    LBASSERT( identifier.isUUID( ));
    impl_->id = identifier;
}

const uint128_t& Object::getID() const
{
    return impl_->id;
}

uint32_t Object::getInstanceID() const
{
    return impl_->instanceID;
}

void Object::attach( const uint128_t& id, const uint32_t instanceID )
{
    LBASSERT( !isAttached() );
    LBASSERT( impl_->localNode );
    LBASSERT( instanceID <= CO_INSTANCE_MAX );

    impl_->id         = id;
    impl_->instanceID = instanceID;
    LBLOG( LOG_OBJECTS )
        << impl_->id << '.' << impl_->instanceID << ": " << lunchbox::className( this )
        << (isMaster() ? " master" : " slave") << std::endl;
}

void Object::detach()
{
    impl_->instanceID = CO_INSTANCE_INVALID;
    impl_->localNode = 0;
}

void Object::notifyDetach()
{
    if( !isMaster( ))
        return;

    // unmap slaves
    const Nodes slaves = impl_->cm->getSlaveNodes();
    if( slaves.empty( ))
        return;

    LBLOG( LOG_BUG ) << slaves.size() << " slaves subscribed during "
                     << " deregister/unmap of " << lunchbox::className( this )
                     << " id " << impl_->id << std::endl;

    for( NodesCIter i = slaves.begin(); i != slaves.end(); ++i )
    {
        NodePtr node = *i;
        node->send( CMD_NODE_UNMAP_OBJECT ) << impl_->id;
    }
}

void Object::transfer( Object* from )
{
    impl_->id           = from->impl_->id;
    impl_->instanceID   = from->getInstanceID();
    impl_->cm           = from->impl_->cm;
    impl_->localNode    = from->impl_->localNode;
    impl_->cm->setObject( this );

    from->impl_->cm = ObjectCM::ZERO;
    from->impl_->localNode = 0;
    from->impl_->instanceID = CO_INSTANCE_INVALID;
}

void Object::_setChangeManager( ObjectCMPtr cm )
{
    if( impl_->cm != ObjectCM::ZERO )
    {
        LBVERB
            << "Overriding existing object change manager, obj "
            << lunchbox::className( this ) << ", old cm "
            << lunchbox::className( impl_->cm ) << ", new cm "
            << lunchbox::className( cm ) << std::endl;
    }

    impl_->cm->exit();
    impl_->cm = cm;
    cm->init();
    LBLOG( LOG_OBJECTS ) << "set " << lunchbox::className( cm ) << " for "
                         << lunchbox::className( this ) << std::endl;
}

ObjectCMPtr Object::_getChangeManager()
{
    return impl_->cm;
}

ObjectOCommand Object::send( NodePtr node, const uint32_t cmd,
                             const uint32_t instanceID )
{
    Connections connections( 1, node->getConnection( ));
    return ObjectOCommand( connections, cmd, COMMANDTYPE_OBJECT, impl_->id,
                           instanceID );
}

void Object::push( const uint128_t& groupID, const uint128_t& typeID,
                   const Nodes& nodes )
{
    impl_->cm->push( groupID, typeID, nodes );
}

uint128_t Object::commit( const uint32_t incarnation )
{
    return impl_->cm->commit( incarnation );
}


void Object::setupChangeManager( const Object::ChangeType type,
                                 const bool master, LocalNodePtr localNode,
                                 const uint32_t masterInstanceID )
{
    impl_->localNode = localNode;

    switch( type )
    {
        case Object::NONE:
            LBASSERT( !localNode );
            _setChangeManager( ObjectCM::ZERO );
            break;

        case Object::STATIC:
            LBASSERT( impl_->localNode );
            if( master )
                _setChangeManager( new StaticMasterCM( this ));
            else
                _setChangeManager( new StaticSlaveCM( this ));
            break;

        case Object::INSTANCE:
            LBASSERT( impl_->localNode );
            if( master )
                _setChangeManager( new FullMasterCM( this ));
            else
                _setChangeManager( new VersionedSlaveCM( this,
                                                         masterInstanceID ));
            break;

        case Object::DELTA:
            LBASSERT( impl_->localNode );
            if( master )
                _setChangeManager( new DeltaMasterCM( this ));
            else
                _setChangeManager( new VersionedSlaveCM( this,
                                                         masterInstanceID ));
            break;

        case Object::UNBUFFERED:
            LBASSERT( impl_->localNode );
            if( master )
                _setChangeManager( new UnbufferedMasterCM( this ));
            else
                _setChangeManager( new VersionedSlaveCM( this,
                                                         masterInstanceID ));
            break;

        default: LBUNIMPLEMENTED;
    }
}

//---------------------------------------------------------------------------
// ChangeManager forwarders
//---------------------------------------------------------------------------
void Object::applyMapData( const uint128_t& version )
{
    impl_->cm->applyMapData( version );
}

void Object::sendInstanceData( Nodes& nodes )
{
    impl_->cm->sendInstanceData( nodes );
}

bool Object::isBuffered() const
{
    return impl_->cm->isBuffered();
}

bool Object::isMaster() const
{
    return impl_->cm->isMaster();
}

void Object::removeSlave( NodePtr node, const uint32_t instanceID )
{
    impl_->cm->removeSlave( node, instanceID );
}

void Object::removeSlaves( NodePtr node )
{
    impl_->cm->removeSlaves( node );
}

void Object::setMasterNode( NodePtr node )
{
    impl_->cm->setMasterNode( node );
}

void Object::addInstanceDatas( const ObjectDataIStreamDeque& cache,
                               const uint128_t& version )
{
    impl_->cm->addInstanceDatas( cache, version );
}

void Object::setAutoObsolete( const uint32_t count )
{
    impl_->cm->setAutoObsolete( count );
}

uint32_t Object::getAutoObsolete() const
{
    return impl_->cm->getAutoObsolete();
}

uint128_t Object::sync( const uint128_t& version )
{
    if( version == VERSION_NONE )
        return getVersion();
    return impl_->cm->sync( version );
}

uint128_t Object::getHeadVersion() const
{
    return impl_->cm->getHeadVersion();
}

uint128_t Object::getVersion() const
{
    return impl_->cm->getVersion();
}

void Object::notifyNewHeadVersion( const uint128_t& version LB_UNUSED )
{
    LBASSERTINFO( getVersion() == VERSION_NONE ||
                  version < getVersion() + 100,
                  lunchbox::className( this ));
}

uint32_t Object::chooseCompressor() const
{
    return pression::Compressor::choose( Global::getPluginRegistry(),
                                         EQ_COMPRESSOR_DATATYPE_BYTE, 1.f,
                                         false );
}

uint32_t Object::getMasterInstanceID() const
{
    return impl_->cm->getMasterInstanceID();
}

NodePtr Object::getMasterNode()
{
    return impl_->cm->getMasterNode();
}

std::ostream& operator << ( std::ostream& os, const Object& object )
{
    os << lunchbox::className( &object ) << " " << object.getID() << "."
       << object.getInstanceID() << " v" << object.getVersion();
    return os;
}

std::ostream& operator << ( std::ostream& os,const Object::ChangeType& type )
{
    return os << ( type == Object::NONE ? "none" :
                   type == Object::STATIC ? "static" :
                   type == Object::INSTANCE ? "instance" :
                   type == Object::DELTA ? "delta" :
                   type == Object::UNBUFFERED ? "unbuffered" : "ERROR" );
}

}
