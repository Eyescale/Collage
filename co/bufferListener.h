
/* Copyright (c) 2012, Stefan.Eilemann@epfl.ch
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

#ifndef CO_BUFFERLISTENER_H
#define CO_BUFFERLISTENER_H

namespace co
{
    /** A listener interface to buffer state changes. */
    class BufferListener
    {
    public:
        virtual ~BufferListener() {}

        virtual void notifyFree( Buffer* ) = 0; //!< No references left
    };
}

#endif // CO_BUFFER_LISTENER_H
