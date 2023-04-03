#pragma once
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include "Util.hpp"
#define BACKLOG 5

class TcpServer
{
public:
    static TcpServer *GetInstance(uint16_t port)
    {
        static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
        if (_instance == nullptr)
        {
            // 加锁
            pthread_mutex_lock(&_mutex);
            if (_instance == nullptr)
            {
                _instance = new TcpServer(port);
                _instance->InitServer();
            }
            // 解锁
            pthread_mutex_unlock(&_mutex);
        }

        return _instance;
    }

    ~TcpServer()
    {
        if(_listensock >= 0)
        {
            close(_listensock);
            Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "close listen sockfd success!");
        }
    }

public:
    void InitServer()
    {
        Socket();
        Bind();
        Listen();
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "tcpserver initializer success!");
    }

    int Accept(struct sockaddr_in *peer, socklen_t *len)
    {
        int sockfd = accept(_listensock, reinterpret_cast<struct sockaddr *>(&peer), len);
        return sockfd;
    }

private:
    TcpServer(uint16_t port)
        : _port(port)
    {
    }

    TcpServer(const TcpServer &obj) = delete;
    TcpServer &operator=(const TcpServer &obj) = delete;

    void Socket()
    {
        _listensock = socket(PF_INET, SOCK_STREAM, 0);
        if (_listensock < 0)
        {
            // 打印日志
            Util::logMessage(FATAL, "filename : %s | line : %d | %s", __FILE__, __LINE__, "create socket error!");
            exit(1);
        }

        int opt = 1;
        // 防止服务器因为异常主动断开的timewait状态
        setsockopt(_listensock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "create socket success!");
    }

    void Bind()
    {
        struct sockaddr_in serv;
        socklen_t len = sizeof(serv);
        memset(&serv, 0, len);
        serv.sin_family = AF_INET;
        serv.sin_port = ntohs(_port);
        serv.sin_addr.s_addr = INADDR_ANY; // 云服务器不能直接绑定公网IP
        int ret = bind(_listensock, reinterpret_cast<struct sockaddr *>(&serv), len);
        if (ret < 0)
        {
            // 打印日志
            Util::logMessage(FATAL, "filename : %s | line : %d | %s", __FILE__, __LINE__, "bind socket error!");
            exit(2);
        }
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "bind socket success!");
    }

    void Listen()
    {
        int ret = listen(_listensock, BACKLOG);
        if (ret < 0)
        {
            // 打印日志
            Util::logMessage(FATAL, "filename : %s | line : %d | %s", __FILE__, __LINE__, "listen socket error!");
            exit(3);
        }
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "listen socket success!");
    }

private:
    uint16_t _port;
    int _listensock;
    static TcpServer *_instance;
};

TcpServer *TcpServer::_instance = nullptr;