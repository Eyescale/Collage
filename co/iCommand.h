
/* Copyright (c) 2006-2014, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_ICOMMAND_H
#define CO_ICOMMAND_H

#include <co/api.h>
#include <co/commands.h>        // for enum CommandType
#include <co/dataIStream.h>     // base class


namespace co
{
namespace detail { class ICommand; }

/**
 * A class managing received commands.
 *
 * This class is used by the LocalNode to pass received buffers to the
 * Dispatcher and ultimately command handler functions. It is not intended to be
 * instantiated by applications. The derivates of this ICommand have to be
 * instaniated by the application if the command type requires it. The data
 * retrieval is possible with the provided DataIStream methods or with the
 * templated get() function.
 */
class ICommand : public DataIStream
{
public:
    CO_API ICommand(); //!< @internal
    CO_API ICommand( LocalNodePtr local, NodePtr remote,
                     ConstBufferPtr buffer, const bool swap ); //!<@internal
    CO_API ICommand( const ICommand& rhs ); //!< @internal

    CO_API virtual ~ICommand(); //!< @internal

    CO_API ICommand & operator = ( const ICommand& rhs ); //!< @internal

    CO_API void clear(); //!< @internal

    /** @name Data Access */
    //@{
    /** @return the command type. @version 1.0 */
    CO_API uint32_t getType() const;

    /** @return the command. @version 1.0 */
    CO_API uint32_t getCommand() const;

    /** @return the command payload size. @version 1.0 */
    CO_API uint64_t getSize() const;

    /** @deprecated use read() */
    template< typename T > T get() { return read< T >(); }

    /** @deprecated use getRemoteNode() */
    NodePtr getNode() const { return getRemoteNode(); }

    /** @return the sending node proxy instance. @version 1.1.1 */
    CO_API NodePtr getRemoteNode() const override;

    /** @return the receiving node. @version 1.0 */
    CO_API LocalNodePtr getLocalNode() const override;

    /** @return true if the command has valid data. @version 1.0 */
    CO_API bool isValid() const;

    /** @internal @return the buffer */
    CO_API ConstBufferPtr getBuffer() const;
    //@}

    /** @internal @name Command dispatch */
    //@{
    /** @internal Change the command type for subsequent dispatching. */
    CO_API void setType( const CommandType type );

    /** @internal Change the command for subsequent dispatching. */
    CO_API void setCommand( const uint32_t cmd );

    /** @internal Set the function to which the command is dispatched. */
    void setDispatchFunction( const Dispatcher::Func& func );

    /** @internal Invoke and clear the command function. */
    CO_API bool operator()();
    //@}

private:
    detail::ICommand* const _impl;

    friend CO_API std::ostream& operator << (std::ostream&,const ICommand&);

    /** @internal @name DataIStream functions */
    //@{
    CO_API size_t nRemainingBuffers() const override;
    CO_API uint128_t getVersion() const override;
    CO_API bool getNextBuffer( uint32_t&, uint32_t&, const void**,
                               uint64_t& ) override;
    //@}

    void _skipHeader(); //!< @internal
};

CO_API std::ostream& operator << ( std::ostream& os, const ICommand& );
}
#endif // CO_ICOMMAND_H
