#include <iostream>
#include <string>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <sys/time.h>
#include <fcntl.h>

using namespace std;
#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535

/*客户端socket地址、待写到客户端数据的位置、从客户端读入的数据*/
struct client_data
{
    struct sockaddr_in address;
    string write_buffer;
    char buf[BUFFER_SIZE];
};

int setnonblock(int sockfd)
{
    int oldoption = fcntl(sockfd, F_GETFL);
    int newoption = oldoption | SOCK_NONBLOCK;
    fcntl(sockfd, F_SETFL, newoption);
    return oldoption;
}

int main(int argc, char *argv[])
{
    if (argc != 2 && argc != 3)
    {
        cout << argv[0] << " port"
             << " ip" << endl;
        return 1;
    }

    string ip;
    short port;
    if (argc == 3)
    {
        ip = argv[2];
    }
    port = stoi(argv[1]);

    int listensock = socket(AF_INET, SOCK_STREAM, 0);

    int ret = 0;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    ip.empty() ? server_addr.sin_addr.s_addr = INADDR_ANY : inet_aton(ip.c_str(), &server_addr.sin_addr);
    ret = bind(listensock, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr));
    assert(ret != -1);

    ret = listen(listensock, 5); // 理解这里5的意思

    assert(ret != -1);
    client_data *users = new client_data[FD_LIMIT];
    // 为listensock占位一个
    struct pollfd fds[USER_LIMIT + 1];
    int user_counter = 0;
    for (int i = 0; i <= USER_LIMIT; ++i)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }

    fds[0].fd = listensock;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while (true)
    {
        ret = poll(fds, user_counter + 1, -1); // 阻塞
        if (ret < 0)
        {
            cout << "poll failure" << endl;
            break;
        }

        for (int i = 0; i < user_counter + 1; ++i)
        {
            if (fds[i].fd == listensock && fds[i].revents & POLLIN)
            {
                struct sockaddr_in peer;
                memset(&peer, 0, sizeof(peer));
                socklen_t peer_len = sizeof(peer);
                int connfd = accept(listensock, reinterpret_cast<struct sockaddr *>(&peer), &peer_len);
                if (connfd < 0)
                {
                    cout << "accept failure" << endl;
                    continue;
                }

                // 如果请求太多，则关闭新到的连接
                if (user_counter >= USER_LIMIT)
                {
                    string info = "too many user";
                    cout << info << endl;
                    send(connfd, info.c_str(), info.size(), 0);
                    close(connfd);
                    continue;
                }

                // 获取到一个新连接
                user_counter++;
                users[connfd].address = peer;
                // 设置connfd为非阻塞
                int oldoption = setnonblock(connfd);

                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                cout << "comes a new client, now have " << user_counter << " clients" << endl;
            }
            else if (fds[i].revents & POLLERR)
            {
                cout << "get a error from " << fds[i].fd << endl;
                char errors[100];
                memset(errors, '\0', sizeof errors);
                socklen_t errors_len = sizeof errors;

                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, errors, &errors_len) < 0)
                {
                    cout << "get socket option is failure" << endl;
                    printf("errors : %s\n", errors);
                }
                continue;
            }
            else if (fds[i].revents & POLLRDHUP)
            {
                cout << "client close sockfd" << endl;
                // 把最后一个user移动到要关闭的连接，以方便有新user连接可以之间放到后面
                close(fds[i].fd);
                users[fds[i].fd] = users[fds[user_counter].fd]; // 浅拷贝就行
                fds[i] = fds[user_counter];
                i--; // 如果不这么做，当前文件描述符下次循环会被错过
                user_counter--;
            }
            else if (fds[i].revents & POLLIN)
            {
                int connfd = fds[i].fd;
                memset(users[connfd].buf, '\0', BUFFER_SIZE);
                ssize_t s = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
                if (s < 0)
                {
                    // 读取操作出错
                    if (errno != EAGAIN)
                    {
                        close(connfd);
                        // 把最后一个user移动到要关闭的连接，以方便有新user连接可以之间放到后面
                        users[fds[i].fd] = users[fds[user_counter].fd]; // 浅拷贝就行
                        fds[i] = fds[user_counter];
                        i--; // 如果不这么做，当前文件描述符下次循环会被错过
                        user_counter--;
                    }
                }
                else if (s == 0)
                {
                }
                else
                {
                    users[connfd].buf[s] = '\0';
                    printf("get %d bytes of client data %s from %d\n", s, users[connfd].buf, connfd);

                    for(int j = 1; j <= user_counter; ++j)
                    {
                        if(fds[j].fd == connfd)
                        {
                            continue;
                        }
                        users[fds[j].fd].write_buffer = users[fds[j].fd].buf;
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                    }
                }
            }
            else if (fds[i].revents & POLLOUT)
            {
                int connfd = fds[i].fd;
                if(users[connfd].write_buffer.empty())
                {
                    continue;
                }
                cout << "send begin" << users[connfd].write_buffer << endl;
                send(connfd, (char *)users[connfd].write_buffer.c_str(), users[connfd].write_buffer.size(), 0);
                users[connfd].write_buffer.clear();

                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete[] users;
    close(listensock);
    return 0;
}