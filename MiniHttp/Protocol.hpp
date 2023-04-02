#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "Util.hpp"

class Entrance
{
public:
    static void *HandelRequest(void *arg)
    {
        int sockfd = *(static_cast<int *>(arg));
        delete static_cast<int *>(arg);

        std::cout << "get a new link... : " << sockfd << std::endl;
        // #ifndef DEBUG
        // #define DEBGU
        //     char buffer[4096];
        //     recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        //     std::cout << "----------begin-------------" << std::endl;
        //     std::cout << buffer << std::endl;
        //     std::cout << "----------end---------------" << std::endl;
        //     // 1.读取请求
        //     // 2.分析请求
        //     // 3.构建响应报文
        // #endif
        std::string line;
        Util::ReadLine(sockfd, &line);
        close(sockfd);
        return nullptr;
    }
};