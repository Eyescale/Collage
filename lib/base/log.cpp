
/* Copyright (c) 2005, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#include "log.h"

#include "perThread.h"

using namespace eqBase;

#ifndef NDEBUG
Clock eqLogClock;
#endif

static int getLogLevel();

int eqBase::Log::level = getLogLevel();
static PerThread<Log*> _logInstance;

int getLogLevel()
{
    const char *env = getenv("EQLOGLEVEL");
    if( env != NULL )
    {
        if( strcmp( env, "ERROR" ) == 0 )
            return LOG_ERROR;
        if( strcmp( env, "WARN" ) == 0 )
            return LOG_WARN;
        if( strcmp( env, "INFO" ) == 0 )
            return LOG_INFO;
        if( strcmp( env, "VERB" ) == 0 )
            return LOG_VERBATIM;
    }

#ifdef NDEBUG
    return LOG_WARN;
#else
    return LOG_INFO;
#endif
}

Log& Log::instance()
{
    if( !_logInstance.get( ))
        _logInstance = new Log();

    return *_logInstance.get();
}

std::ostream& eqBase::indent( std::ostream& os )
{
    Log* log = dynamic_cast<Log*>(&os);
    if( log )
        log->indent();
    return os;
}
std::ostream& eqBase::exdent( std::ostream& os )
{
    Log* log = dynamic_cast<Log*>(&os);
    if( log )
        log->exdent();
        return os;
}
