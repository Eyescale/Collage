
/* Copyright (c) 2007-2016, Stefan Eilemann <eile@equalizergraphics.com>
 *                          Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_OBJECTDATAISTREAM_H
#define CO_OBJECTDATAISTREAM_H

#include <co/iCommand.h>        // member
#include <co/dataIStream.h>     // base class
#include <co/version.h>         // enum
#include <lunchbox/monitor.h>   // member
#include <lunchbox/thread.h>    // member

#include <deque>

namespace co
{
/** The DataIStream for object data. */
class ObjectDataIStream : public DataIStream
{
public:
    ObjectDataIStream();
    ObjectDataIStream( const ObjectDataIStream& rhs );
    virtual ~ObjectDataIStream();

    ObjectDataIStream& operator = ( const ObjectDataIStream& rhs );

    void addDataCommand( ObjectDataICommand command );
    size_t getDataSize() const;

    uint128_t getVersion() const override { return _version.get(); }
    uint128_t getPendingVersion() const;

    void waitReady() const { _version.waitNE( VERSION_INVALID ); }
    bool isReady() const { return _version != VERSION_INVALID; }

    size_t nRemainingBuffers() const override { return _commands.size(); }

    void reset() override;

    bool hasInstanceData() const;

    CO_API NodePtr getRemoteNode() const override;
    CO_API LocalNodePtr getLocalNode() const override;

protected:
    bool getNextBuffer( CompressorInfo& compressor, uint32_t& nChunks,
                        const void*& chunkData, uint64_t& size ) override;

private:
    typedef std::deque< ICommand > CommandDeque;

    /** All data commands for this istream. */
    CommandDeque _commands;

    ICommand _usedCommand; //!< Currently used buffer

    /** The object version associated with this input stream. */
    lunchbox::Monitor< uint128_t > _version;

    void _setReady() { _version = getPendingVersion(); }
    void _reset();

    LB_TS_VAR( _thread );
};
}
#endif //CO_OBJECTDATAISTREAM_H
