#pragma once 

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

//工具类
class Util
{
public:
    static int ReadLine(int sockfd, std::string *line)
    {
        char ch = 'X';
        while(ch != '\n')
        {
            ssize_t s = recv(sockfd, &ch, 1, 0);
            if(s > 0)
            {
                
            }
            else if(s == 0)
            {
                return 0;
            }
            else 
            {
                return -1;
            }
        }
    }
};