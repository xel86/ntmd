#include "APIController.hpp"

#include "traffic/DBController.hpp"
#include "traffic/TrafficStorage.hpp"
#include "util/HumanReadable.hpp"

#include <nlohmann/json.hpp>

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

APIController::APIController(TrafficStorage& trafficStorage, const DBController& db) :
    mTrafficStorage(trafficStorage), mDB(db)
{
    this->startSocketServer();
}

void APIController::startSocketServer()
{
    const int mPort = 13889;

    int serverFd;
    sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Failed to create the server's socket file descriptor for API. Proceeding "
                     "without API functionality.\n";
        return;
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << "Failed to attach the server's socket to the port " << mPort
                  << " for API. Proceeding "
                     "without API functionality.\n";
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(mPort);

    if (bind(serverFd, (sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "Failed to bind the server's socket to the port " << mPort
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
                    << "Error while attempting to listen onto server socket file descriptor. \n";
                /* TODO: don't break here, handle errors */
                continue;
            }

            if ((newSocket = accept(serverFd, (sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
            {
                std::cerr << "Error accepting new incoming socket request.\n";
                continue;
            }

            valread = read(newSocket, buffer, sizeof(buffer));
            buffer[valread] = '\0';

            std::string request = std::string(buffer);
            std::cerr << "GOT API REQUEST: " << request << "\n";

            /* Handle API requests.
             * API Handlers are responsible for closing the socket belonging to the requester. */
            if (request == "live-text")
            {
                this->liveText(newSocket);
            }
            else if (request == "live")
            {
                this->live(newSocket);
            }
            else if (request == "snapshot")
            {
                this->snapshot(newSocket);
            }
            else
            {
                const char* msg = "Unknown request.";
                send(newSocket, msg, strlen(msg), 0);
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
            std::unordered_map<std::string, TrafficLine> trafficMap;
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
            std::unordered_map<std::string, TrafficLine> trafficMap;
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

            json payload;
            payload["length"] = trafficMap.size();
            payload["result"] = "success";

            for (const auto& [name, line] : trafficMap)
            {
                payload["data"][name] = {
                    {"bytesRx", line.bytesRx},       {"bytesTx", line.bytesTx},
                    {"pktRxCount", line.pktRxCount}, {"pktTxCount", line.pktTxCount},
                    {"interval", interval},
                };
            }

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

    const std::unordered_map<std::string, TrafficLine>& trafficMap = trafficSnapshot.first;
    const int& interval = trafficSnapshot.second;

    std::stringstream ss;

    json payload;
    payload["length"] = trafficMap.size();
    payload["result"] = "success";

    for (const auto& [name, line] : trafficMap)
    {
        payload["data"][name] = {
            {"bytesRx", line.bytesRx},       {"bytesTx", line.bytesTx},
            {"pktRxCount", line.pktRxCount}, {"pktTxCount", line.pktTxCount},
            {"interval", interval},
        };
    }

    std::string msg = payload.dump();

    send(socketfd, msg.c_str(), msg.size(), MSG_NOSIGNAL);

    close(socketfd);
}

} // namespace ntmd