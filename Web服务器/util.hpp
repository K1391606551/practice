#pragma once
#include <iostream>
#include <exception>
#include <cerrno>
#include <cassert>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

// 定义服务器响应状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherehtly impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Find";
const char* error_404_form = "The request file was not fount on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unsusal problem serving the reuqested file.\n";

// 网站根目录
const char *doc_root = "/val/www/html";

struct Util
{
public:
    struct Epoll
    {
    public:
        static int CreateEpoll()
        {
            int epollfd = epoll_create(128);
            if (epollfd < 0)
            {
                throw std::exception();
            }

            return epollfd;
        }

        static bool Addevent(int epfd, int sockfd, uint32_t event)
        {
            struct epoll_event ev;
            ev.events = event;
            ev.data.fd = sockfd;
            return epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) == 0;
        }

        static bool Modevent(int epfd, int sockfd, uint32_t event)
        {
            struct epoll_event ev;
            ev.events = event;
            ev.data.fd = sockfd;
            return epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev) == 0;
        }

        static bool Delevent(int epfd, int sockfd)
        {
            return epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr) == 0;
        }

        static int Getevents(int epfd, struct epoll_event *ready_list, int max_number)
        {
            int number = epoll_wait(epfd, ready_list, max_number, -1);
            return number;
        }
    };

    struct SetNonBlock
    {
        static int setnonblock(int sockfd)
        {
            int old_option = fcntl(sockfd, F_GETFL);
            int new_option = old_option | O_NONBLOCK;
            fcntl(sockfd, F_SETFL, new_option);

            return old_option;
        }
    };
};