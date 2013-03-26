
/* Copyright (c) 2005-2013, Stefan Eilemann <eile@equalizergraphics.com>
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

#include "init.h"

#include "global.h"
#include "node.h"
#include "socketConnection.h"

#include <lunchbox/init.h>
#include <lunchbox/os.h>
#include <lunchbox/pluginRegistry.h>

namespace co
{
namespace
{
static int32_t _checkVersion()
{
    static std::string version = Version::getString();
    if( version != Version::getString( ))
        LBWARN << "Duplicate DSO loading, Collage v" << version
               << " already loaded while loading v" << Version::getString()
               << std::endl;
    return 0;
}

static lunchbox::a_int32_t _initialized( _checkVersion( ));
}

bool _init( const int argc, char** argv )
{
    if( ++_initialized > 1 ) // not first
        return true;

    if( !lunchbox::init( argc, argv ))
        return false;

    // init all available plugins
    lunchbox::PluginRegistry& plugins = Global::getPluginRegistry();
    plugins.addLunchboxPlugins();
    plugins.addDirectory( "/opt/local/lib" ); // MacPorts
    plugins.init();

#ifdef _WIN32
    WORD    wsVersion = MAKEWORD( 2, 0 );
    WSADATA wsData;
    if( WSAStartup( wsVersion, &wsData ) != 0 )
    {
        LBERROR << "Initialization of Windows Sockets failed"
                << lunchbox::sysError << std::endl;
        return false;
    }
#endif

    return true;
}

bool exit()
{
    if( --_initialized > 0 ) // not last
        return true;
    LBASSERT( _initialized == 0 );

#ifdef _WIN32
    if( WSACleanup() != 0 )
    {
        LBERROR << "Cleanup of Windows Sockets failed"
                << lunchbox::sysError << std::endl;
        return false;
    }
#endif

    // de-initialize registered plugins
    lunchbox::PluginRegistry& plugins = Global::getPluginRegistry();
    plugins.exit();

    return lunchbox::exit();
}

}
