#pragma once

#include "traffic/DBController.hpp"
#include "traffic/TrafficStorage.hpp"

namespace ntmd {

class APIController
{
  public:
    APIController(TrafficStorage& trafficStorage, const DBController& db);
    ~APIController() = default;

  private:
    /* Spawns a new thread to wait for and handle incoming socket API requests. */
    void startSocketServer();

    /* API Commands */
    void live(int socketfd);

    TrafficStorage& mTrafficStorage;
    const DBController& mDB;
};

} // namespace ntmd