
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

#ifndef CO_FULLMASTERCM_H
#define CO_FULLMASTERCM_H

#include "objectInstanceDataOStream.h" // member
#include "versionedMasterCM.h"         // base class

#include <deque>

namespace co
{
class ObjectDataIStream;

/**
 * An object change manager handling only full versions for the master
 * instance.
 * @internal
 */
class FullMasterCM : public VersionedMasterCM
{
public:
    explicit FullMasterCM(Object* object);
    virtual ~FullMasterCM();

    void init() override;
    uint128_t commit(const uint32_t incarnation) override;
    void push(const uint128_t& groupID, const uint128_t& typeID,
              const Nodes& nodes) override;
    bool sendSync(const MasterCMCommand& command) override;

    /** @name Versioning */
    //@{
    void setAutoObsolete(const uint32_t count) override;
    uint32_t getAutoObsolete() const override { return _nVersions; }
    //@}

    /** Speculatively send instance data to all nodes. */
    void sendInstanceData(const Nodes& nodes) override;

protected:
    struct InstanceData
    {
        explicit InstanceData(const VersionedMasterCM* cm)
            : os(cm)
            , commitCount(0)
        {
        }

        ObjectInstanceDataOStream os;
        uint32_t commitCount;
    };

    bool _initSlave(const MasterCMCommand&, const uint128_t&, bool) override;

    InstanceData* _newInstanceData();
    void _addInstanceData(InstanceData* data);
    void _releaseInstanceData(InstanceData* data);

    void _updateCommitCount(const uint32_t incarnation);
    void _obsolete();
    void _checkConsistency() const;

    bool isBuffered() const override { return true; }
    virtual void _commit();

private:
    /** The number of commits, needed for auto-obsoletion. */
    uint32_t _commitCount;

    /** The number of old versions to retain. */
    uint32_t _nVersions;

    typedef std::deque<InstanceData*> InstanceDataDeque;
    typedef std::vector<InstanceData*> InstanceDatas;

    /** The list of full instance datas, head version last. */
    InstanceDataDeque _instanceDatas;
    InstanceDatas _instanceDataCache;

    /* The command handlers. */
    bool _cmdCommit(ICommand& command);
    bool _cmdObsolete(ICommand& command);
    bool _cmdPush(ICommand& command);
};
}

#endif // CO_FULLMASTERCM_H
