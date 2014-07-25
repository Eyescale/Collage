
/* Copyright (c) 2005-2014, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_INIT_H
#define CO_INIT_H

#include <co/api.h>
#include <co/version.h>  // used inline
#include <lunchbox/log.h> // used inline

namespace co
{
/** @internal */
CO_API bool _init( const int argc, char** argv );

/**
 * Initialize the Collage network library.
 *
 * exit() should be called independent of the return value of this function.
 *
 * @param argc the command line argument count.
 * @param argv the command line argument values.
 * @return true if the library was successfully initialised, false otherwise
 * @version 1.0
 */
inline bool init( const int argc, char** argv )
{
    if( CO_VERSION_ABI == Version::getABI( ))
        return co::_init( argc, argv );
    LBWARN << "Collage shared library v" << Version::getABI()
           << " not binary compatible with application v" << CO_VERSION_ABI
           << std::endl;
    return false;
}

/**
 * De-initialize the Collage network library.
 *
 * @return true if the library was successfully de-initialised,
 *         false otherwise.
 * @version 1.0
 */
CO_API bool exit();
}

#endif // CO_INIT_H
