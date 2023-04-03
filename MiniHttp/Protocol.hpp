#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "Util.hpp"

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
    std::string _url;
    std::string _version;
    std::unordered_map<std::string, std::string> _header_kv;
    int _content_length = 0;
};

class HttpResponse
{
public:
    std::string _response_line;
    std::vector<std::string> _response_header;
    std::string _blank_line;
    std::string _response_content;
public:
    int _status_code = OK;
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
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, _http_request._method.c_str());
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, _http_request._url.c_str());
        Util::logMessage(INFO, "filename : %s | line : %d | %s", __FILE__, __LINE__, _http_request._version.c_str());
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
        // 解析完读取content
        RecvHttpRequestContent();
    }

    void BuildHttpResponse()
    {
        auto &code = _http_response._status_code;
        if (_http_request._method != "GET" || _http_request._method != "POST")
        {
            Util::logMessage(WARNING, "filename : %s | line : %d | %s", __FILE__, __LINE__, "bad request!");
            code = NOT_FOUND;
            goto END;
        }
        END:
        
    }

    void SendHttpResponse()
    {
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
        // 3.构建响应报文
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