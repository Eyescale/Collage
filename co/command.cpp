
/* Copyright (c) 2006-2012, Stefan Eilemann <eile@equalizergraphics.com> 
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

#include "command.h"

#include "node.h"

namespace co
{
namespace detail
{
class Command
{
public:
    Command( lunchbox::a_int32_t& freeCounter )
        : _node()
        , _dataSize( 0 )
        , _master()
        , _freeCount( freeCounter )
        , _func( 0, 0 )
    {}

    NodePtr _node; //!< The node sending the packet
    uint64_t _dataSize; //!< The size of the allocation
    CommandPtr _master;
    lunchbox::a_int32_t& _freeCount;
    co::Dispatcher::Func _func;
};
}

Command::Command( lunchbox::a_int32_t& freeCounter )
    : lunchbox::Referenced()
    , _impl( new detail::Command( freeCounter ))
    , _packet( 0 )
    , _data( 0 )
{}

Command::~Command() 
{
    free();
    delete _impl;
}

void Command::deleteReferenced( const Referenced* object ) const
{
    Command* command = const_cast< Command* >( this );
    // DON'T 'command->_master = 0;', command is already reusable and _master
    // may be set any time. alloc or clone_ will free old master.
    ++command->_impl->_freeCount;
}

size_t Command::alloc_( NodePtr node, const uint64_t size )
{
    LB_TS_THREAD( _writeThread );
    LBASSERT( getRefCount() == 1 ); // caller CommandCache
    LBASSERTINFO( !_impl->_func.isValid(), *this );
    LBASSERT( _impl->_freeCount > 0 );

    --_impl->_freeCount;

    size_t allocated = 0;
    if( !_data )
    {
        _impl->_dataSize = LB_MAX( Packet::minSize, size );
        _data = static_cast< Packet* >( malloc( _impl->_dataSize ));
        allocated = _impl->_dataSize;
    }
    else if( size > _impl->_dataSize )
    {
        allocated =  size - _impl->_dataSize;
        _impl->_dataSize = LB_MAX( Packet::minSize, size );
        ::free( _data );
        _data = static_cast< Packet* >( malloc( _impl->_dataSize ));
    }

    _impl->_node = node;
    _impl->_func.clear();
    _packet = _data;
    _packet->size = size;
    _impl->_master = 0;

    return allocated;
}

void Command::clone_( CommandPtr from )
{
    LB_TS_THREAD( _writeThread );
    LBASSERT( getRefCount() == 1 ); // caller CommandCache
    LBASSERT( from->getRefCount() > 1 ); // caller CommandCache, self
    LBASSERTINFO( !_impl->_func.isValid(), *this );
    LBASSERT( _impl->_freeCount > 0 );

    --_impl->_freeCount;

    _impl->_node = from->_impl->_node;
    _packet = from->_packet;
    _impl->_master = from;
}

void Command::free()
{
    LB_TS_THREAD( _writeThread );
    LBASSERT( !_impl->_func.isValid( ));

    if( _data )
        ::free( _data );

    _data = 0;
    _impl->_dataSize = 0;
    _packet = 0;
    _impl->_node = 0;
    _impl->_master = 0;
}

NodePtr Command::getNode() const
{
    return _impl->_node;
}

uint64_t Command::getAllocationSize() const
{
    return _impl->_dataSize;
}

void Command::setDispatchFunction( const Dispatcher::Func& func )
{
    LBASSERT( !_impl->_func.isValid( ));
    _impl->_func = func;
}

bool Command::operator()()
{
    LBASSERT( _impl->_func.isValid( ));
    Dispatcher::Func func = _impl->_func;
    _impl->_func.clear();
    return func( this );
}

std::ostream& operator << ( std::ostream& os, const Command& command )
{
    if( command.isValid( ))
    {
        os << lunchbox::disableFlush << "command< ";
        const Packet* packet = command.get< Packet >() ;
        os << packet;
        os << ", " << command.getNode() << " >" << lunchbox::enableFlush;
    }
    else
        os << "command< empty >";

    if( command._impl->_func.isValid( ))
        os << ' ' << command._impl->_func << std::endl;
    return os;
}
}
