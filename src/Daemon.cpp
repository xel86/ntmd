#include "Daemon.hpp"

#include <csignal>
#include <iostream>

namespace ntmd {

/* systemd log levels */
const char* logcritical = "<2> ";
const char* logerror = "<3> ";
const char* logwarn = "<4> ";
const char* lognotice = "<5> ";
const char* loginfo = "<6> ";
const char* logdebug = "<7> ";

Daemon::Daemon()
{
    signal(SIGINT, Daemon::signalHandler);
    signal(SIGTERM, Daemon::signalHandler);
    signal(SIGHUP, Daemon::signalHandler);
}

bool Daemon::running()
{
    if (mReload)
    {
        mReload = false;
        reload();
    }

    return mIsRunning;
}

void Daemon::reload()
{
    // TODO implement reloading config. maybe?
}

void Daemon::signalHandler(int signal)
{
    switch (signal)
    {
    case SIGINT:
    case SIGTERM: {
        Daemon::instance().mIsRunning = false;
        break;
    }
    case SIGHUP: {
        Daemon::instance().mReload = true;
        break;
    }
    }
}

} // namespace ntmd
