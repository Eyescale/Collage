
/* Copyright (c) 2010-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "versionedMasterCM.h"

#include "log.h"
#include "object.h"
#include "objectCommand.h"
#include "objectDataICommand.h"
#include "objectDataIStream.h"

namespace co
{
typedef CommandFunc<VersionedMasterCM> CmdFunc;

VersionedMasterCM::VersionedMasterCM( Object* object )
        : ObjectCM( object )
        , _version( VERSION_NONE )
        , _maxVersion( std::numeric_limits< uint64_t >::max( ))
{
    LBASSERT( object );
    LBASSERT( object->getLocalNode( ));

    // sync commands are send to all instances, even the master gets it
    object->registerCommand( CMD_OBJECT_INSTANCE,
                             CmdFunc( this, &VersionedMasterCM::_cmdDiscard ),
                             0 );
    object->registerCommand( CMD_OBJECT_DELTA,
                             CmdFunc( this, &VersionedMasterCM::_cmdDiscard ),
                             0 );

    object->registerCommand( CMD_OBJECT_SLAVE_DELTA,
                            CmdFunc( this, &VersionedMasterCM::_cmdSlaveDelta ),
                             0 );
    object->registerCommand( CMD_OBJECT_MAX_VERSION,
                            CmdFunc( this, &VersionedMasterCM::_cmdMaxVersion ),
                             0 );
}

VersionedMasterCM::~VersionedMasterCM()
{
    _slaves->clear();
}

uint128_t VersionedMasterCM::sync( const uint128_t& inVersion )
{
    LBASSERTINFO( inVersion.high() != 0 || inVersion == VERSION_NEXT ||
                  inVersion == VERSION_HEAD, inVersion );
#if 0
    LBLOG( LOG_OBJECTS ) << "sync to v" << inVersion << ", id "
                         << _object->getID() << "." << _object->getInstanceID()
                         << std::endl;
#endif

    if( inVersion == VERSION_NEXT )
        return _apply( _slaveCommits.pop( ));

    if( inVersion == VERSION_HEAD )
    {
        uint128_t version = VERSION_NONE;
        for( ObjectDataIStream* is = _slaveCommits.tryPop(); is;
             is = _slaveCommits.tryPop( ))
        {
            version = _apply( is );
        }
        return version;
    }
    // else apply only concrete slave commit

    return _apply( _slaveCommits.pull( inVersion ));
}

uint128_t VersionedMasterCM::_apply( ObjectDataIStream* is )
{
    LBASSERT( !is->hasInstanceData( ));
    _object->unpack( *is );
    LBASSERTINFO( is->getRemainingBufferSize() == 0 &&
                  is->nRemainingBuffers()==0,
                  "Object " << lunchbox::className( _object ) <<
                  " did not unpack all data" );

    const uint128_t version = is->getVersion();
    is->reset();
    _slaveCommits.recycle( is );
    return version;
}

bool VersionedMasterCM::addSlave( const MasterCMCommand& command )
{
    LB_TS_THREAD( _cmdThread );
    Mutex mutex( _slaves );

    if( !ObjectCM::_addSlave( command, _version ))
        return false;

    SlaveData data;
    data.node = command.getNode();
    data.instanceID = command.getInstanceID();
    data.maxVersion = command.getMaxVersion();
    if( data.maxVersion == 0 )
        data.maxVersion = std::numeric_limits< uint64_t >::max();
    else if( data.maxVersion < std::numeric_limits< uint64_t >::max( ))
        data.maxVersion += _version.low();

    _slaveData.push_back( data );
    _updateMaxVersion();

    _slaves->push_back( data.node );
    lunchbox::usort( *_slaves );
    return true;
}

void VersionedMasterCM::removeSlave( NodePtr node, const uint32_t instanceID )
{
    LB_TS_THREAD( _cmdThread );
    Mutex mutex( _slaves );

    // remove from subscribers
    SlaveData data;
    data.node = node;
    data.instanceID = instanceID;
    SlaveDatasIter i = lunchbox::find( _slaveData, data );
    LBASSERTINFO( i != _slaveData.end(), lunchbox::className( _object ));
    if( i == _slaveData.end( ))
        return;

    _slaveData.erase( i );

    // update _slaves node vector
    _slaves->clear();
    for( i = _slaveData.begin(); i != _slaveData.end(); ++i )
        _slaves->push_back( i->node );
    lunchbox::usort( *_slaves );
    _updateMaxVersion();
}

void VersionedMasterCM::removeSlaves( NodePtr node )
{
    LB_TS_THREAD( _cmdThread );

    Mutex mutex( _slaves );

    NodesIter i = lunchbox::find( *_slaves, node );
    if( i == _slaves->end( ))
        return;
    _slaves->erase( i );

    for( SlaveDatasIter j = _slaveData.begin(); j != _slaveData.end(); )
    {
        if( j->node == node )
            j = _slaveData.erase( j );
        else
            ++j;
    }
    _updateMaxVersion();
}

void VersionedMasterCM::_updateMaxVersion()
{
    uint64_t maxVersion = std::numeric_limits< uint64_t >::max();
    for( SlaveDatasCIter i = _slaveData.begin(); i != _slaveData.end(); ++i )
    {
        if( i->maxVersion != std::numeric_limits< uint64_t >::max() &&
            maxVersion > i->maxVersion )
        {
            maxVersion = i->maxVersion;
        }
    }

    if( _maxVersion != maxVersion )
       _maxVersion = maxVersion;
}

//---------------------------------------------------------------------------
// command handlers
//---------------------------------------------------------------------------
bool VersionedMasterCM::_cmdSlaveDelta( ICommand& cmd )
{
    ObjectDataICommand command( cmd );

    LB_TS_THREAD( _rcvThread );

    if( _slaveCommits.addDataCommand( command.get< uint128_t >(), command ))
        _object->notifyNewVersion();
    return true;
}

bool VersionedMasterCM::_cmdMaxVersion( ICommand& cmd )
{
    ObjectICommand command( cmd );
    const uint64_t version = command.get< uint64_t >();
    const uint32_t slaveID = command.get< uint32_t >();

    Mutex mutex( _slaves );

    // Update slave's max version
    SlaveData data;
    data.node = command.getNode();
    data.instanceID = slaveID;
    SlaveDatasIter i = lunchbox::find( _slaveData, data );
    if( i == _slaveData.end( ))
    {
        LBWARN << "Got max version from unmapped slave" << std::endl;
        return true;
    }

    i->maxVersion = version;
    _updateMaxVersion();
    return true;
}

}
