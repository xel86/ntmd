#include "APIController.hpp"

#include "Daemon.hpp"
#include "traffic/DBController.hpp"
#include "traffic/TrafficStorage.hpp"
#include "util/HumanReadable.hpp"
#include "util/StringUtil.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

using json = nlohmann::json;

namespace ntmd {

APIController::APIController(TrafficStorage& trafficStorage, const DBController& db,
                             uint16_t port) :
    mTrafficStorage(trafficStorage),
    mDB(db), mPort(port)
{
    this->startSocketServer();
}

void APIController::startSocketServer()
{
    int serverFd;
    sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << ntmd::logerror
                  << "Failed to create the server's socket file descriptor for API. Proceeding "
                     "without API functionality.\n";
        return;
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << ntmd::logerror << "Failed to attach the server's socket to the port " << mPort
                  << " for API. Proceeding "
                     "without API functionality.\n";
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(mPort);

    if (bind(serverFd, (sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << ntmd::logerror << "Failed to bind the server's socket to the port " << mPort
                  << " for API. Proceeding "
                     "without API functionality.\n";
        return;
    }

    std::thread loop([&] {
        int newSocket, valread;
        char buffer[1024] = {0};

        while (true)
        {
            if (listen(serverFd, 5) < 0)
            {
                std::cerr
                    << ntmd::logwarn
                    << "Error while attempting to listen onto server socket file descriptor. \n";
                /* TODO: don't break here, handle errors */
                continue;
            }

            if ((newSocket = accept(serverFd, (sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
            {
                std::cerr << ntmd::logwarn << "Error accepting new incoming socket request.\n";
                continue;
            }

            valread = read(newSocket, buffer, sizeof(buffer));
            buffer[valread] = '\0';

            std::string strbuf = std::string(buffer);
            std::vector<std::string> request = util::split(strbuf);

            const std::string& cmd = request[0];

            /* Handle API requests.
             * API Handlers are responsible for closing the socket belonging to the requester. */
            if (cmd == "live-text")
            {
                this->liveText(newSocket);
            }
            else if (cmd == "live")
            {
                this->live(newSocket);
            }
            else if (cmd == "snapshot")
            {
                this->snapshot(newSocket);
            }
            else if (cmd == "traffic-daily")
            {
                this->trafficDaily(newSocket);
            }
            else if (cmd == "traffic-since")
            {
                if (request.size() >= 2)
                {
                    /* Expected Parameters: time_t ts */
                    time_t ts;

                    try
                    {
                        ts = std::stoi(request[1]);
                    }
                    catch (const std::invalid_argument& ia)
                    {
                        json err;
                        err["result"] = "error";
                        err["errmsg"] = "Invalid timestamp parameter for traffic-since.";
                        std::string msg = err.dump();

                        send(newSocket, msg.c_str(), msg.size(), 0);
                        close(newSocket);
                        continue;
                    }
                    catch (const std::out_of_range& oor)
                    {
                        json err;
                        err["result"] = "error";
                        err["errmsg"] = "Timestamp parameter value too large for traffic-since.";
                        std::string msg = err.dump();

                        send(newSocket, msg.c_str(), msg.size(), 0);
                        close(newSocket);
                        continue;
                    }

                    this->trafficSince(newSocket, ts);
                }
                else
                {
                    json err;
                    err["result"] = "error";
                    err["errmsg"] = "Missing timestamp parameter for traffic-since.";
                    std::string msg = err.dump();

                    send(newSocket, msg.c_str(), msg.size(), 0);
                    close(newSocket);
                }
            }
            else if (cmd == "traffic-between")
            {
                if (request.size() >= 3)
                {
                    /* Expected Parameters: time_t start, end */
                    time_t start, end;

                    try
                    {
                        start = std::stoi(request[1]);
                        end = std::stoi(request[2]);
                    }
                    catch (const std::invalid_argument& ia)
                    {
                        json err;
                        err["result"] = "error";
                        err["errmsg"] = "Invalid timestamp parameter(s) for traffic-between.";
                        std::string msg = err.dump();

                        send(newSocket, msg.c_str(), msg.size(), 0);
                        close(newSocket);
                        continue;
                    }
                    catch (const std::out_of_range& oor)
                    {
                        json err;
                        err["result"] = "error";
                        err["errmsg"] =
                            "Timestamp parameter value(s) too large for traffic-between.";
                        std::string msg = err.dump();

                        send(newSocket, msg.c_str(), msg.size(), 0);
                        close(newSocket);
                        continue;
                    }

                    this->trafficBetween(newSocket, start, end);
                }
                else
                {
                    json err;
                    err["result"] = "error";
                    err["errmsg"] = "Missing timestamp parameter(s) for traffic-between.";
                    std::string msg = err.dump();

                    send(newSocket, msg.c_str(), msg.size(), 0);
                    close(newSocket);
                }
            }
            else
            {
                json err;
                err["result"] = "error";
                err["errmsg"] = "Unknown command.";
                std::string msg = err.dump();

                send(newSocket, msg.c_str(), msg.size(), 0);
                close(newSocket);
            }
        }

        shutdown(serverFd, SHUT_RDWR);
    });
    loop.detach();
}

void APIController::liveText(int socketfd)
{
    std::thread updates([this, socketfd] {
        const char* welcomeMsg =
            "Connected to ntmd live traffic update stream, awaiting first traffic interval.\n";

        /* If remote peer closes connection, return. */
        if (send(socketfd, welcomeMsg, strlen(welcomeMsg), MSG_NOSIGNAL) < 0)
        {
            return;
        }

        while (true)
        {
            TrafficMap trafficMap;
            int interval;
            std::mutex await;

            /* Send pointers to our trafficMap and interval variables to the traffic storage so that
             * it can set them with the updated values at the proper time. This will also lock the
             * mutex sent in. */
            if (!mTrafficStorage.awaitSnapshot(await, trafficMap, interval))
            {
                const char* msg = "Only one live API stream at a time is currently supported.";
                send(socketfd, msg, strlen(msg), MSG_NOSIGNAL);
                break;
            }

            /* Wait until the traffic storage has set our variables and unlocked the mutex. */
            await.lock();

            std::stringstream ss;
            ss << "Application Traffic:\n";
            for (const auto& [name, line] : trafficMap)
            {
                ss << "  ";
                ss << name << " { rx: " << util::bytesToHumanOvertime(line.bytesRx, interval)
                   << ", tx: " << util::bytesToHumanOvertime(line.bytesTx, interval)
                   << ", rxc: " << line.pktRxCount << ", txc: " << line.pktTxCount << " }\n";
            }

            std::string msg = ss.str();

            /* If remote peer closes connection, break. */
            if (send(socketfd, msg.c_str(), msg.size(), MSG_NOSIGNAL) < 0)
            {
                break;
            }
        }
        close(socketfd);
    });
    updates.detach();
}

void APIController::live(int socketfd)
{
    std::thread updates([this, socketfd] {
        while (true)
        {
            TrafficMap trafficMap;
            int interval;
            std::mutex await;

            /* Send pointers to our trafficMap and interval variables to the traffic storage so that
             * it can set them with the updated values at the proper time. This will also lock the
             * mutex sent in. */
            if (!mTrafficStorage.awaitSnapshot(await, trafficMap, interval))
            {
                json err;
                err["result"] = "error";
                err["errmsg"] = "Only one live API stream at a time is currently supported.";
                std::string msg = err.dump();

                send(socketfd, msg.c_str(), msg.size(), 0);
                break;
            }

            /* Wait until the traffic storage has set our variables and unlocked the mutex. */
            await.lock();

            json payload = trafficToJson(trafficMap);
            payload["interval"] = interval;
            payload["result"] = "success";

            std::string msg = payload.dump();

            /* If remote peer closes connection, break. */
            if (send(socketfd, msg.c_str(), msg.size(), MSG_NOSIGNAL) < 0)
            {
                break;
            }
        }
        close(socketfd);
    });
    updates.detach();
}

void APIController::snapshot(int socketfd)
{
    auto trafficSnapshot = mTrafficStorage.getLiveSnapshot();

    const TrafficMap& trafficMap = trafficSnapshot.first;
    const int& interval = trafficSnapshot.second;

    json payload = trafficToJson(trafficMap);
    payload["interval"] = interval;
    payload["result"] = "success";

    std::string msg = payload.dump();

    send(socketfd, msg.c_str(), msg.size(), MSG_NOSIGNAL);

    close(socketfd);
}

void APIController::trafficDaily(int socketfd)
{
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    auto day = std::localtime(&now);
    day->tm_hour = 0;
    day->tm_min = 0;
    day->tm_sec = 0;

    time_t startOfCurrentDay = std::mktime(day);

    auto trafficMap = mDB.fetchTrafficSince(startOfCurrentDay);

    json payload = trafficToJson(trafficMap);
    payload["result"] = "success";

    std::string msg = payload.dump();

    send(socketfd, msg.c_str(), msg.size(), MSG_NOSIGNAL);

    close(socketfd);
}

void APIController::trafficSince(int socketfd, time_t ts)
{
    auto trafficMap = mDB.fetchTrafficSince(ts);

    json payload = trafficToJson(trafficMap);
    payload["result"] = "success";

    std::string msg = payload.dump();

    send(socketfd, msg.c_str(), msg.size(), MSG_NOSIGNAL);

    close(socketfd);
}

void APIController::trafficBetween(int socketfd, time_t start, time_t end)
{
    auto trafficMap = mDB.fetchTrafficBetween(start, end);

    json payload = trafficToJson(trafficMap);
    payload["result"] = "success";

    std::string msg = payload.dump();

    send(socketfd, msg.c_str(), msg.size(), MSG_NOSIGNAL);

    close(socketfd);
}

json APIController::trafficToJson(const TrafficMap& traffic)
{
    json payload;

    payload["length"] = traffic.size();
    for (const auto& [name, line] : traffic)
    {
        payload["data"][name] = {
            {"bytesRx", line.bytesRx},
            {"bytesTx", line.bytesTx},
            {"pktRxCount", line.pktRxCount},
            {"pktTxCount", line.pktTxCount},
        };
    }

    return payload;
}

} // namespace ntmd