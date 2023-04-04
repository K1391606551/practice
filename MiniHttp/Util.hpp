#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <cstdarg>
#include <ctime>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define INFO 0
#define WARNING 1
#define ERROR 2
#define FATAL 3

const char *log_level[] = {"INFO", "WARNING", "ERROR", "FATAL"};

// 工具类
class Util
{
public:
    // 按行读取函数
    static int ReadLine(int sockfd, std::string *line)
    {
        char ch = 'X';
        while (ch != '\n')
        {
            ssize_t s = recv(sockfd, &ch, 1, 0);
            if (s > 0)
            {
                if (ch == '\r')
                {
                    // 窥探
                    recv(sockfd, &ch, 1, MSG_PEEK);
                    if (ch == '\n')
                    {
                        // 窥探成功
                        ssize_t s = recv(sockfd, &ch, 1, 0);
                    }
                    else
                    {
                        ch = '\n';
                    }
                }

                // 1.普通字符
                // 2.'\n'

                (*line) += ch;
            }
            else if (s == 0)
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }

        return (*line).size();
    }

    // 日志函数
    static void logMessage(int level, const char *format, ...)
    {
        assert(level >= INFO);
        assert(level <= FATAL);

        char *name = getenv("USER");

        char logInfo[1024];
        va_list ap; // ap -> char*
        va_start(ap, format);

        vsnprintf(logInfo, sizeof(logInfo) - 1, format, ap);

        va_end(ap); // ap = NULL

        FILE *out = (level == FATAL) ? stderr : stdout;
        fprintf(out, "%s | %u | %s | %s\n",
                log_level[level],
                (unsigned int)time(nullptr),
                name == nullptr ? "unknow" : name,
                logInfo);

        fflush(out);        // 将C缓冲区中的数据刷新到OS
        fsync(fileno(out)); // 将OS中的数据尽快刷盘
    }

    static void Cutstring(const std::string &target, std::string *sub1, std::string *sub2, const std::string &sep)
    {
        size_t pos = target.find(sep);
        if(pos != std::string::npos)
        {
            *sub1 = target.substr(0, pos);
            *sub2 = target.substr(pos + sep.size());
        }
    }

    static std::string Code2Desc(int code)
    {
        std::string desc;
        switch(code)
        {
            case 200:
                desc = "OK";
                break;
            case 404:
                desc = "Not Found";
                break;
            default:
                break;
        }

        return desc;
    }

    static std::string Suffix2Desc(const std::string &suffix)
    {
        std::unordered_map<std::string, std::string> suffix2desc = {
            {".html", "text/html"},
            {".css", "text/css"},
            {"js", "application/javascript"},
            {".jpg", "application/x-jpg"},
            {".htm", "text/htm"}
        };
        
        auto iter = suffix2desc.find(suffix);
        if(iter != suffix2desc.end())
        {
            return iter->second;
        }
        return "text/html";
    }
};