
/* Copyright (c) 2006-2016, Stefan Eilemann <eile@equalizergraphics.com>
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

#ifndef CO_LOG_H
#define CO_LOG_H

#include <pression/log.h>

namespace co
{
    /** lunchbox::Log topic emitted by Collage. */
    enum LogTopics
    {
        LOG_BUG     = lunchbox::LOG_BUG,          // 4
        LOG_OBJECTS = pression::LOG_CUSTOM << 0,  // 16
        LOG_BARRIER = pression::LOG_CUSTOM << 1,  // 32
        LOG_RSP     = pression::LOG_CUSTOM << 2,  // 64
        LOG_PACKETS = pression::LOG_CUSTOM << 3,  // 128
        LOG_CUSTOM  = pression::LOG_CUSTOM << 5   // 512
    };
}
#endif // CO_LOG_H
