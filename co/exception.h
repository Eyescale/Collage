
/* Copyright (c) 2011, Cedric Stalder <cedric.stalder@gmail.com>
 *               2012-2013, Stefan.Eilemann@epfl.ch
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

#ifndef CO_EXCEPTION_H
#define CO_EXCEPTION_H
#include <sstream>

namespace co
{
    class Exception;
    std::ostream& operator << ( std::ostream& os, const Exception& e );

    /** A base Exception for Collage operations. */
    class Exception : public std::exception
    {
    public:
        /** The exception type. @version 1.0 */
        enum Type
        {
            TIMEOUT_WRITE,   //!< A write timeout operation
            TIMEOUT_READ,    //!< A read timeout operation
            TIMEOUT_BARRIER, //!< A barrier timeout operation
            TIMEOUT_COMMANDQUEUE, //!< A timeout on a cmd queue operation
            CUSTOM      = 20 //!< Application-specific exceptions
        };

        /** Construct a new Exception. @version 1.0 */
        Exception( const uint32_t type ) : _type( type ) {}

        /** Destruct this exception. @version 1.0 */
        virtual ~Exception() throw() {}

        /** @return the type of this exception @version 1.0 */
        virtual uint32_t getType() const { return _type; }

        /** Output the exception in human-readable form. @version 1.0 */
        virtual const char* what() const throw()
        {
            switch( _type )
            {
              case Exception::TIMEOUT_WRITE:
                  return " Timeout on write operation";
              case Exception::TIMEOUT_READ:
                  return " Timeout on read operation";
              case Exception::TIMEOUT_BARRIER:
                  return " Timeout on barrier";
              case Exception::TIMEOUT_COMMANDQUEUE:
                  return " Timeout on command queue";
              default:
                  return  " Unknown Exception";
            }
        }

    private:
        /** The type of this eception instance **/
        const uint32_t _type;
    };

    /** Output the exception in human-readable form. @version 1.0 */
    inline std::ostream& operator << ( std::ostream& os, const Exception& e )
        { return os << e.what(); }
}

#endif // CO_EXCEPTION_H
