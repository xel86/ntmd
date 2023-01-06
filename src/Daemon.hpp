#pragma once

namespace ntmd {

/* systemd log levels */
extern const char* logcritical;
extern const char* logerror;
extern const char* logwarn;
extern const char* lognotice;
extern const char* loginfo;
extern const char* logdebug;

class Daemon
{
  public:
    static Daemon& instance()
    {
        static Daemon instance;
        return instance;
    }

    bool running();

  private:
    Daemon();
    Daemon(Daemon const&) = delete;
    void operator=(Daemon const&) = delete;

    void reload();
    static void signalHandler(int signal);

    bool mIsRunning{true};
    bool mReload{false};
};

} // namespace ntmd