#pragma once

#include "traffic/DBController.hpp"
#include "traffic/TrafficStorage.hpp"

#include <nlohmann/json.hpp>

#include <ctime>

using json = nlohmann::json;

namespace ntmd {

class APIController
{
    using TrafficMap = std::unordered_map<std::string, TrafficLine>;

  public:
    APIController(TrafficStorage& trafficStorage, const DBController& db, uint16_t port);
    ~APIController() = default;

  private:
    /* Spawns a new thread to wait for and handle incoming socket API requests. */
    void startSocketServer();

    /* API Commands */
    void liveText(int socketfd);
    void live(int socketfd);
    void snapshot(int socketfd);

    void trafficDaily(int socketfd);
    void trafficSince(int socketfd, time_t ts);
    void trafficBetween(int socketfd, time_t start, time_t end);

    /* Helpers */
    json trafficToJson(const TrafficMap& traffic);

    TrafficStorage& mTrafficStorage;
    const DBController& mDB;
    uint16_t mPort{13889};
};

} // namespace ntmd