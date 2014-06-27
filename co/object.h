
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

#ifndef CO_OBJECT_H
#define CO_OBJECT_H

#include <co/dispatcher.h>    // base class
#include <co/localNode.h>     // used in RefPtr
#include <co/types.h>         // for Nodes
#include <co/version.h>       // used as default parameter
#include <lunchbox/bitOperation.h> // byteswap inline impl

namespace co
{
namespace detail { class Object; }
class ObjectCM;
typedef lunchbox::RefPtr< ObjectCM > ObjectCMPtr;

#  define CO_COMMIT_NEXT LB_UNDEFINED_UINT32 //!< the next commit incarnation

/**
 * A distributed object.
 *
 * Please refer to the Equalizer Programming Guide and examples on how to
 * develop and use distributed objects. The Serializable implements a typical
 * common use case based on the basic Object.
 */
class Object : public Dispatcher
{
public:
    /** Object change handling characteristics, see Programming Guide */
    enum ChangeType
    {
        NONE,              //!< @internal
        STATIC,            //!< non-versioned, unbuffered, static object.
        INSTANCE,          //!< use only instance data
        DELTA,             //!< use pack/unpack delta
        UNBUFFERED         //!< versioned, but don't retain versions
    };

    /** Destruct the distributed object. @version 1.0 */
    CO_API virtual ~Object();

    /** @name Data Access */
    //@{
    /**
     * @return true if the object is attached (mapped or registered).
     * @version 1.0
     */
    CO_API bool isAttached() const;

    /** @return the local node to which this object is attached. @version 1.0 */
    CO_API LocalNodePtr getLocalNode();

    /**
     * Set the object's unique identifier.
     *
     * Only to be called on unattached objects. The application has to
     * ensure the uniqueness of the identifier in the peer-to-peer node
     * network. By default, each object has an identifier guaranteed to be
     * unique. During mapping, the identifier of the object will be
     * overwritten with the identifier of the master object.
     * @version 1.0
     */
    CO_API void setID( const uint128_t& identifier );

    /** @return the object's unique identifier. @version 1.0 */
    CO_API const uint128_t& getID() const;

    /** @return the node-wide unique object instance identifier. @version 1.0 */
    CO_API uint32_t getInstanceID() const;

    /** @internal @return if this object keeps instance data buffers. */
    CO_API bool isBuffered() const;

    /**
     * @return true if this instance is a registered master version.
     * @version 1.0
     */
    CO_API bool isMaster() const;
    //@}

    /** @name Versioning */
    //@{
    /** @return how the changes are to be handled. @version 1.0 */
    virtual ChangeType getChangeType() const { return STATIC; }

    /**
     * Limit the number of queued versions on slave instances.
     *
     * Changing the return value of this method causes the master instance
     * to block during commit() if any slave instance has reached the
     * maximum number of queued versions. The method is called on the slave
     * instance. Multiple slave instances may use different values.
     *
     * Changing the return value at runtime, that is, after the slave
     * instance has been mapped is unsupported and causes undefined
     * behavior.
     *
     * Not supported on master instances for slave object commits. Open an
     * issue if you need this.
     *
     * @return the number of queued versions a slave instance may have.
     * @version 1.0
     */
    virtual uint64_t getMaxVersions() const
        { return std::numeric_limits< uint64_t >::max(); }

    /**
     * Return the compressor to be used for data transmission.
     *
     * This default implementation chooses the compressor with the highest
     * lossless compression ratio for EQ_COMPRESSOR_DATATYPE_BYTE tokens. The
     * application may override this method to deactivate compression by
     * returning EQ_COMPRESSOR_NONE or to select object-specific compressors.
     * @version 1.0
     */
    CO_API virtual uint32_t chooseCompressor() const;

    /**
     * Return if this object needs a commit.
     *
     * This function is used for optimization, to detect early that no
     * commit is needed. If it returns true, pack() or getInstanceData()
     * will be executed. The serialization methods can still decide to not
     * write any data, upon which no new version will be created. If it
     * returns false, commit() will exit early.
     *
     * @return true if a commit is needed.
     * @version 1.0
     */
    virtual bool isDirty() const { return true; }

    /**
     * Push the instance data of the object to the given nodes.
     *
     * Used to push object data from a Node, instead of pulling it during
     * mapping. Does not establish any mapping, that is, the receiving side
     * will typically use LocalNode::mapObject with VERSION_NONE to
     * establish a slave mapping. LocalNode::objectPush() is called once for
     * each node in the nodes list.
     *
     * Buffered objects do not reserialize their instance data. Multicast
     * connections are preferred, that is, for a set of nodes sharing one
     * multicast connection the data is only send once.
     *
     * @param groupID An identifier to group a set of push operations.
     * @param objectType A per-push identifier.
     * @param nodes The vector of nodes to push to.
     * @version 1.0
     */
    CO_API void push( const uint128_t& groupID, const uint128_t& objectType,
                      const Nodes& nodes );

    /**
     * Commit a new version of this object.
     *
     * Objects using the change type STATIC can not be committed.
     *
     * Master instances will increment new versions continuously, starting at
     * VERSION_FIRST. If the object has not changed, no new version will
     * be generated, that is, the current version is returned. The high
     * value of the returned version will always be 0.
     *
     * Slave objects can be committed, but have certain caveats for
     * serialization. Please refer to the Equalizer Programming Guide for
     * details. Slave object commits will return a random version on a
     * successful commit, or VERSION_NONE if the object has not changed since
     * the last commit. The high value of a successful slave commit will never
     * be 0.
     *
     * The incarnation count is meaningful for buffered master objects. The
     * commit implementation will keep all instance data committed with an
     * incarnation count newer than <code>current_incarnation -
     * getAutoObsolete()</code>. By default, each call to commit creates a new
     * incarnation, retaining the data from last getAutoObsolete() commit
     * calls. When the application wishes to auto obsolete by another metric
     * than commit calls, it has to consistently provide a corresponding
     * incarnation counter. Buffers with a higher incarnation count than the
     * current are discarded. A typical use case is to tie the auto obsoletion
     * to an application-specific frame loop. Decreasing the incarnation counter
     * will lead to undefined results.
     *
     * @param incarnation the commit incarnation for auto obsoletion.
     * @return the new head version (master) or commit id (slave).
     * @version 1.0
     */
    CO_API virtual uint128_t commit( const uint32_t incarnation =
                                     CO_COMMIT_NEXT );

    /**
     * Automatically obsolete old versions.
     *
     * The versions for the last count incarnations are retained for the
     * buffered object types INSTANCE and DELTA.
     *
     * @param count the number of incarnations to retain.
     * @version 1.0
     */
    CO_API void setAutoObsolete( const uint32_t count );

    /** @return get the number of retained incarnations. @version 1.0 */
    CO_API uint32_t getAutoObsolete() const;

    /**
     * Sync to a given version.
     *
     * Objects using the change type STATIC can not be synced.
     *
     * Syncing to VERSION_HEAD syncs all received versions, does not block and
     * always returns true. Syncing to VERSION_NEXT applies one new version,
     * potentially blocking. Syncing to VERSION_NONE does nothing.
     *
     * Slave objects can be synced to VERSION_HEAD, VERSION_NEXT or to any past
     * or future version generated by a commit on the master instance. Syncing
     * to a concrete version applies all pending versions up to this version and
     * potentially blocks if given a future version.
     *
     * Master objects can only be synced to VERSION_HEAD, VERSION_NEXT or to
     * any version generated by a commit on a slave instance. Syncing to a
     * concrete version applies only this version and potentially blocks.
     *
     * The different sync semantics for concrete versions originate from the
     * fact that master versions are continous and ordered, while slave
     * versions are random and unordered.
     *
     * This function is not thread safe, that is, calling sync()
     * simultaneously on the same object from multiple threads has to be
     * protected by the caller using a mutex.
     *
     * @param version the version to synchronize (see above).
     * @return the last version applied.
     * @version 1.0
     */
    CO_API virtual uint128_t sync( const uint128_t& version = VERSION_HEAD );

    /** @return the latest available (head) version. @version 1.0 */
    CO_API uint128_t getHeadVersion() const;

    /** @return the currently synchronized version. @version 1.0 */
    CO_API uint128_t getVersion() const;

    /**
     * Notification that a new head version was received by a slave object.
     *
     * The notification is called from the receiver thread, which is different
     * from the node main thread. The object cannot be sync()'ed from this
     * notification, as this might lead to deadlocks and synchronization issues
     * with the application thread changing the object. It should signal the
     * application, which then takes the appropriate action. Do not perform any
     * potentially blocking operations from this method.
     *
     * @param version The new head version.
     * @version 1.0
     */
    CO_API virtual void notifyNewHeadVersion( const uint128_t& version );

    /**
     * Notification that a new version was received by a master object.
     * The same constraints as for notifyNewHeadVersion() apply.
     * @version 1.0
     */
    virtual void notifyNewVersion() {}
    //@}

    /** @name Serialization methods for instantiation and versioning. */
    //@{
    /**
     * Serialize all instance information of this distributed object.
     *
     * @param os The output stream.
     * @version 1.0
     */
    virtual void getInstanceData( DataOStream& os ) = 0;

    /**
     * Deserialize the instance data.
     *
     * This method is called during object mapping to populate slave
     * instances with the master object's data.
     *
     * @param is the input stream.
     * @version 1.0
     */
    virtual void applyInstanceData( DataIStream& is ) = 0;

    /**
     * Serialize the modifications since the last call to commit().
     *
     * No new version will be created if no data is written to the output
     * stream.
     *
     * @param os the output stream.
     * @version 1.0
     */
    virtual void pack( DataOStream& os ) { getInstanceData( os ); }

    /**
     * Deserialize a change.
     *
     * @param is the input data stream.
     * @version 1.0
     */
    virtual void unpack( DataIStream& is ) { applyInstanceData( is ); }
    //@}

    /** @name Messaging API */
    //@{
    /**
     * Send a command with optional data to object instance(s) on another
     * node.
     *
     * The returned command can be used to pass additional data. The data
     * will be send after the command object is destroyed, aka when it is
     * running out of scope.
     *
     * @param node the node where to send the command to.
     * @param cmd the object command to execute.
     * @param instanceID the object instance(s) which should handle the command.
     * @return the command object to pass additional data to.
     * @version 1.0
     */
    CO_API ObjectOCommand send( NodePtr node, const uint32_t cmd,
                                const uint32_t instanceID = CO_INSTANCE_ALL );
    //@}

    /** @name Notifications */
    /**
     * Notify that this object will be registered or mapped.
     *
     * The method is called from the thread initiating the registration or
     * mapping, before the operation is executed.
     * @version 1.0
     */
    virtual void notifyAttach() {}

    /**
     * Notify that this object has been registered or mapped.
     *
     * The method is called from the thread initiating the registration or
     * mapping, after the operation has been completed successfully.
     * @sa isMaster()
     * @version 1.0
     */
    virtual void notifyAttached() {}

    /**
     * Notify that this object will be deregistered or unmapped.
     *
     * The method is called from the thread initiating the deregistration or
     * unmapping, before the operation is executed.
     * @sa isMaster()
     * @version 1.0
     */
    CO_API virtual void notifyDetach();

    /**
     * Notify that this object has been deregistered or unmapped.
     *
     * The method is called from the thread initiating the deregistration or
     * unmapping, after the operation has been executed.
     * @version 1.0
     */
    virtual void notifyDetached() {}
    //@}

    /** @internal */
    //@{
    /** @internal @return the master object instance identifier. */
    uint32_t getMasterInstanceID() const;

    /** @internal @return the master object instance identifier. */
    NodePtr getMasterNode();

    /** @internal */
    CO_API void removeSlave( NodePtr node, const uint32_t instanceID );
    CO_API void removeSlaves( NodePtr node ); //!< @internal
    void setMasterNode( NodePtr node ); //!< @internal
    /** @internal */
    void addInstanceDatas( const ObjectDataIStreamDeque&, const uint128_t&);

    /**
     * @internal
     * Setup the change manager.
     *
     * @param type the type of the change manager.
     * @param master true if this object is the master.
     * @param localNode the node the object will be attached to.
     * @param masterInstanceID the instance identifier of the master object,
     *                         used when master == false.
     */
    void setupChangeManager( const Object::ChangeType type,
                             const bool master, LocalNodePtr localNode,
                             const uint32_t masterInstanceID );

    /** @internal Called when object is attached from the receiver thread. */
    CO_API virtual void attach( const uint128_t& id,
                                const uint32_t instanceID );
    /**
     * @internal Called when the object is detached from the local node from the
     * receiver thread.
     */
    CO_API virtual void detach();

    /** @internal Transfer the attachment from the given object. */
    void transfer( Object* from );

    void applyMapData( const uint128_t& version ); //!< @internal
    void sendInstanceData( Nodes& nodes ); //!< @internal
    //@}

protected:
    /** Construct a new distributed object. @version 1.0 */
    CO_API Object();

    /** NOP assignment operator. @version 1.0 */
    Object& operator = ( const Object& ) { return *this; }

    /** Copy construct a new, unattached object. @version 1.0 */
    CO_API Object( const Object& );

private:
    detail::Object* const impl_;
    void _setChangeManager( ObjectCMPtr cm );

    ObjectCMPtr _getChangeManager();
    friend class ObjectStore;

    LB_TS_VAR( _thread );
};

/** Output information about the object to the given stream. @version 1.0 */
CO_API std::ostream& operator << ( std::ostream&, const Object& );

/** Output object change type in human-readably form. @version 1.0 */
CO_API std::ostream& operator << ( std::ostream&, const Object::ChangeType& );
}

namespace lunchbox
{
template<> inline void byteswap( co::Object::ChangeType& value )
{ byteswap( reinterpret_cast< uint32_t& >( value )); }
}
#endif // CO_OBJECT_H
