#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include "Util.hpp"

#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "http/1.0"
#define LINE_END "\r\n"

#define OK 200
#define NOT_FOUND 404
class HttpRequest
{
public:
    // 解析前的内容
    std::string _request_line;
    std::vector<std::string> _request_header;
    std::string _blank_line;
    std::string _request_content;

public:
    // 解析后的内容
    std::string _method;
    std::string _url; // GET path?query
    std::string _version;
    std::unordered_map<std::string, std::string> _header_kv;
    int _content_length = 0;
    std::string _path;
    std::string _query_string;
    std::string _suffix;

    bool _cgi = false;
};

class HttpResponse
{
public:
    std::string _response_line;
    std::vector<std::string> _response_header;
    std::string _blank_line = LINE_END;
    std::string _response_content;

public:
    int _status_code = OK;
    int _fd = -1;
    int _file_size = 0;
};

// 读取请求，分析请求，构建响应
class EndPoint
{
public:
    EndPoint(int sockfd) : _sockfd(sockfd)
    {
    }
    ~EndPoint()
    {
        close(_sockfd);
    }

private:
    void RecvHttpRequestLine()
    {
        std::string &line = _http_request._request_line;
        Util::ReadLine(_sockfd, &line);
        _http_request._request_line.resize(line.size() - 1);
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, _http_request._request_line.c_str());
    }

    void RecvHttpRequestHeader()
    {
        // 不断的按行读，把读到的行内容放入vector当中，直到读到空行为止
        std::string line;
        while (true)
        {
            line.clear();
            Util::ReadLine(_sockfd, &line);
            if (line == "\n")
            {
                _http_request._blank_line = line;
                break;
            }
            line.resize(line.size() - 1);
            _http_request._request_header.push_back(line);
            Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, line.c_str());
        }
    }

    void ParseHttpRequestLine()
    {
        // method url version
        std::string &line = _http_request._request_line;

        // stringstream切分字符串
        std::stringstream ss(line);
        ss >> _http_request._method >> _http_request._url >> _http_request._version;
        // 将方法统一转化为大写
        std::string &method = _http_request._method;
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        // Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, _http_request._method.c_str());
        // Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, _http_request._url.c_str());
        // Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, _http_request._version.c_str());
    }

    void ParseHttpRequestHeader()
    {
        std::string key;
        std::string value;
        std::string sep = ": ";
        for (auto &elem : _http_request._request_header)
        {
            Util::Cutstring(elem, &key, &value, sep);
            // std::cout << key << std::endl;
            // std::cout << value << std::endl;
            _http_request._header_kv.insert({key, value});
        }
    }

    bool IsNeedRecvHttpRequestContent()
    {
        auto &method = _http_request._method;
        if (method == "POST")
        {
            auto &head_kv = _http_request._header_kv;
            auto iter = head_kv.find("Content-Length");
            if (iter != head_kv.end())
            {
                _http_request._content_length = atoi(iter->second.c_str());
            }
            return true;
        }
        return false;
    }

    void RecvHttpRequestContent()
    {
        if (IsNeedRecvHttpRequestContent())
        {
            int content_length = _http_request._content_length;
            auto &content = _http_request._request_content;
            char ch = '\0';
            while (content_length)
            {
                ssize_t s = recv(_sockfd, &ch, 1, 0);
                if (s > 0)
                {
                    content += ch;
                    content_length--;
                }
                else
                {
                    break;
                }
            }
        }
    }

    int ProcessNonCgi()
    {
        // 构建HTTP响应
        // 1.打开目标文件
        _http_response._fd = open(_http_request._path.c_str(), O_RDONLY);
        // 2.构建响应行
        if (_http_response._fd >= 0)
        {
            // 如果打开资源成功了，我们才构建HTTP响应
            _http_response._response_line = HTTP_VERSION;
            _http_response._response_line += " ";
            _http_response._response_line += std::to_string(_http_response._status_code);
            _http_response._response_line += " ";
            _http_response._response_line += Util::Code2Desc(_http_response._status_code);
            _http_response._response_line += LINE_END;

            // 构建响应报文
            std::string header_line = "Content-Type: ";
            header_line += Util::Suffix2Desc(_http_request._suffix);
            header_line += LINE_END;
            _http_response._response_header.push_back(header_line);

            header_line = "Content-Length: ";
            header_line += std::to_string(_http_response._file_size);
            header_line += LINE_END;
            _http_response._response_header.push_back(header_line);

            // 构建空行
            _http_response._blank_line = LINE_END;

            // 构建响应正文
            // 用了sendfile接口，直接在内核中进行拷贝，提高了效率

            return OK;
        }

        // 500 服务器报错
        return 500;
    }

    int ProcessCgi()
    {
        std::cout << "debug: "
                  << "Use CGI Model" << std::endl;
        pid_t pid = fork();
        if (pid == 0)
        {
            // child ->exec*
        }
        else if (pid < 0)
        {
            Util::logMessage(ERROR, "filename : %s | line : %d | %s", __FILE__, __LINE__, "fork fail!");
            return 404;
        }
        else 
        {
            // parent
            waitpid(pid, nullptr, 0);
        }
        return OK;
    }

public:
    void RecvHttpRequest()
    {
        RecvHttpRequestLine();
        RecvHttpRequestHeader();
    }

    // 读取完首行和请求报头就对其进行解析，判断Content-Length是否为零
    // 或者是否有Content-Length字段，是否需要读取请求正文等问题
    void ParseHttpRequest()
    {
        ParseHttpRequestLine();
        ParseHttpRequestHeader();
        // 解析完读取content(内部会自动判断是否有内容需要读取)
        RecvHttpRequestContent();
    }

    void BuildHttpResponse()
    {
        std::string _path;
        int found = 0;
        auto &code = _http_response._status_code;
        auto &method = _http_request._method;
        if (method != "GET" && method != "POST")
        {
            Util::logMessage(WARNING, "filename : %s | line : %d | %s", __FILE__, __LINE__, "bad request!");
            code = NOT_FOUND;
            goto END;
        }
        // 如果时GET方法，需要判断是否带参数了
        // path?query
        if (method == "GET")
        {
            int pos = _http_request._url.find("?");
            if (pos != std::string::npos)
            {
                // 说明带参数了,需要进行提参了
                Util::Cutstring(_http_request._url, &_http_request._path, &_http_request._query_string, "?");
                _http_request._cgi = true;
            }
            else
            {
                // 说明没有带参数
                _http_request._path = _http_request._url;
            }
        }
        else
        {
            _http_request._cgi = true;
        }

        // std::cout << "bebug url: " << _http_request._url << std::endl;
        // std::cout << "bebug path: " << _http_request._path << std::endl;
        // std::cout << "debug query: " << _http_request._query_string << std::endl;

        // 拼接WEB_ROOT目录
        _path = _http_request._path;
        _http_request._path = WEB_ROOT;
        _http_request._path += _path;

        if (_http_request._path[_http_request._path.size() - 1] == '/')
        {
            // 如果用户请求的是一个目录或者根目录，我们要给他反回一个网页
            _http_request._path += HOME_PAGE;
        }
        // std::cout << "debug path: " << _http_request._path << std::endl;

        struct stat st;
        if (!stat(_http_request._path.c_str(), &st))
        {
            // resource exists
            // 判断是否为目录
            if (S_ISDIR(st.st_mode))
            {
                // 是目录
                // 我们要给他反回一个网页
                _http_request._path += "/";
                _http_request._path += HOME_PAGE;
                // 文件属性跟新
                stat(_http_request._path.c_str(), &st);
            }

            // 判断文件是否为可执行
            if (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)
            {
                // 特殊处理
                _http_request._cgi = true;
            }

            _http_response._file_size = st.st_size;
        }
        else
        {
            // 请求资源不存在
            // 404 NOT_FOUNT
            std::string info = _http_request._path;
            info += " Not Found!";
            Util::logMessage(WARNING, "filename : %s | line : %d | %s", __FILE__, __LINE__, info.c_str());
            code = NOT_FOUND;
            goto END;
        }

        // path
        found = _http_request._path.rfind(".");
        if (found == std::string::npos)
        {
            _http_request._suffix = _http_request._path.substr(found);
        }

        // 构建响应报文
        if (_http_request._cgi)
        {
            ProcessCgi();
        }
        else
        {
            // 1.目标网页一定是存在的
            // 2.返回并不是简单的返回网页，而是构建HTTP响应
            code = ProcessNonCgi();
        }
    END:

        if (code != OK)
        {
            // 错误处理
        }
        return;
    }

    void SendHttpResponse()
    {
        // 1. 响应请求行
        send(_sockfd, _http_response._response_line.c_str(), _http_response._response_line.size(), 0);
        // 2. 响应报头
        for (std::string &elem : _http_response._response_header)
        {
            send(_sockfd, elem.c_str(), elem.size(), 0);
        }
        // 3. 空行
        send(_sockfd, _http_response._blank_line.c_str(), _http_response._blank_line.size(), 0);
        // 4. 响应正文
        sendfile(_sockfd, _http_response._fd, nullptr, _http_response._file_size);
        close(_http_response._fd);
    }

private:
    int _sockfd;
    HttpRequest _http_request;
    HttpResponse _http_response;
};

class Entrance
{
public:
    static void *HandelRequest(void *arg)
    {
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "handel request start!");
        int sockfd = *(static_cast<int *>(arg));
        delete static_cast<int *>(arg);

        // #ifndef DEBUG
        // #define DEBGU
        // std::cout << "get a new link... : " << sockfd << std::endl;
        //     char buffer[4096];
        //     recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        //     std::cout << "----------begin-------------" << std::endl;
        //     std::cout << buffer << std::endl;
        //     std::cout << "----------end---------------" << std::endl;
        // 1.读取请求
        // 2.分析请求
        // 3.构建响应报文-
        // #else
        EndPoint *ep = new EndPoint(sockfd);
        ep->RecvHttpRequest();
        ep->ParseHttpRequest();
        ep->BuildHttpResponse();
        ep->SendHttpResponse();
        // #endif
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, "handel request finish!");

        return nullptr;
    }
};