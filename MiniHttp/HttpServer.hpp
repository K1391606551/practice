#pragma once
#include <iostream>
#include "TcpServer.hpp"
#include "Protocol.hpp"
#include "Util.hpp"

#define PORT 8080

class HttpServer
{
public:
    HttpServer(uint16_t port = PORT)
        : _port(port), _tcp_server(nullptr), _stop(false)
    {
    }

    void InitServer()
    {
        _tcp_server = TcpServer::GetInstance(_port);
    }

    void Loop()
    {
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "loop start!");
        while (!_stop)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            memset(&peer, 0, len);
            int sockfd = _tcp_server->Accept(&peer, &len);
            if (sockfd < 0)
            {
                Util::logMessage(WARNING, "filename : %s | line : %d | %s", __FILE__, __LINE__, "accept fail!");
                continue;
            }
            Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "get a new link");
            int *_sock = new int(sockfd);
            pthread_t tid;
            pthread_create(&tid, nullptr, Entrance::HandelRequest, _sock);
            pthread_detach(tid);
        }
    }

private:
    uint16_t _port;
    TcpServer *_tcp_server;
    bool _stop;
};
