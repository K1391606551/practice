#include <iostream>
#include <cstring>
#include <cassert>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

using namespace std;

#define BUFFER_SIZE 64


int main(int argc, char *argv[])
{
    if(argc <= 2)
    {
        cout << argv[0] << " ip" << " port" << endl;
        return 1;
    }

    string ip = argv[1];
    uint16_t port = stoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0 ,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_aton(ip.c_str(), &server_addr.sin_addr);

    int ret = connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(server_addr));
    if(ret < 0)
    {
        close(sockfd);
        return 2;
    }
    
    pollfd fds[2];
    // 注册文件描述符0，和文件描述符sockfd上的读写事件
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char *inbuffer[BUFFER_SIZE];
    int pipefd[2];
    ret = pipe(pipefd);
    assert(ret != -1);
    
    while(true)
    {
        ret = poll(fds, 2, -1);
        if(ret < 0)
        {
            cout << "poll failure" << endl;
        }

        if(fds[1].revents & POLLRDHUP)
        {
            cout << "server close the connection" << endl;
        }
        else if(fds[1].revents & POLLIN)
        {
            memset(inbuffer, '\0', sizeof(inbuffer));
            recv(fds[1].fd, inbuffer, sizeof(inbuffer) - 1, 0);
            cout << inbuffer << endl;
        }

        if(fds[0].revents & POLLIN)
        {
            ret = splice(0, nullptr, pipefd[1], nullptr, 32785, SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], nullptr, sockfd, nullptr, 32785, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    return 0;
}