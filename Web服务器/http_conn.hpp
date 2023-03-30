#pragma once
#include "util.hpp"
#include <cstring>
#include <unordered_map>
#include <unistd.h>
#include <string>

class http_conn
{
public:
    // 文件名的最大长度
    static const int FILENAME_LEN = 200;
    // 读缓冲区的大小
    static const int READ_BUFFER_SIZE = 2048;
    // 写缓存区的大小
    static const int WRITE_BUFFER_SIZE = 1024;
    // HTTP 请求方法--->这里知支持GET
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        PATCH
    };
    // 解析客户请求时，主状态机所处的状态
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    // 服务器处理HTTP请求的可能结果，状态码
    enum HTTP_CODE
    {
        NO_REQUEST /*请求不完整*/,
        GET_REQUEST /*获得了一个完整的客户请求*/,
        BAD_REQUEST /*用户请求出现了语法错误*/,
        NO_RESOURCE /*用户请求的资源不从在*/,
        FORBIDDEN_REQUEST /*用户对请求的资源没有访问权限*/,
        FILE_REQUEST /**/,
        INTERNAL_ERROR,   /*服务器内部报错*/
        CLOSED_CONNECTION /*客户端已经关闭连接*/
    };
    // 行的读取状态
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    // 初始化新接受的连接
    void init(int sockfd, const struct sockadd_in &address)
    {
        // _peer_sockfd_addr.insert(std::make_pair(sockfd, address));
        // 下面两行是为了避免TIME_WAIT状态，仅适用与调试，实际使用时应该去掉
        int op = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &op, sizeof(op));
        Util::Epoll::Addevent(_epollfd, sockfd, EPOLLET | EPOLLIN | EPOLLRDHUP | EPOLLONESHOT);
        int old_option = Util::SetNonBlock::setnonblock(sockfd);
        _user_count++;

        init();
    }
    // 关闭连接
    void close_conn(bool real_close = true)
    {
        if (real_close && (_sockfd != -1))
        {
            Util::Epoll::Delevent(_epollfd, _sockfd);
            close(_sockfd);
            _sockfd = -1;
            _user_count--;
        }
    }
    // 处理客户请求
    void handel()
    {
    }
    // 非阻塞读
    bool read()
    {
        // 循环读取客户数据，直到无数据可读或者对方关闭连接
        if (_read_index >= READ_BUFFER_SIZE)
        {
            return false;
        }

        int bytes_read = 0;
        while (true)
        {
            bytes_read = recv(_sockfd, _read_buffer + _read_index, READ_BUFFER_SIZE - _read_index, 0);
            if (bytes_read == -1)
            {
                // 出内核读缓冲区读完数据而出错
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                // 由于信号而导致读取中断
                else if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    return false;
                }
            }
            else if (bytes_read == 0)
            {
                // 由于对端关闭
                return false;
            }
            _read_index += bytes_read;
        }

        return true;
    }
    // 非阻塞写
    bool write()
    {
    }

private:
    // 初始化连接
    void init()
    {
        _check_state = CHECK_STATE_REQUESTLINE;
        _linger = false;
        _method = GET;
        _url = '\0';
        _version = '\0';
        _content_length = 0;
        _host = '\0';
        _start_line = 0;
        _check_index = 0;
        _read_index = 0;
        _write_index = 0;
        memset(_read_buffer, '\0', READ_BUFFER_SIZE);
        memset(_write_buffer, '\0', WRITE_BUFFER_SIZE);
        memset(_real_file, '\0', FILENAME_LEN);
    }
    // 解析HTTP请求
    HTTP_CODE handel_read()
    {
    }
    // 填充HTTP应答
    bool handel_write(HTTP_CODE ret)
    {
    }

private:
    // 这一组函数用于被handel_read调用分析HTTP请求
    HTTP_CODE parse_request_line(std::string &text)
    {
        int pos1 = text.find(' ');
        int pos2 = text.rfind(' ');
        if (pos1 == pos2)
        {
            return BAD_REQUEST;
        }
        _url = text.substr(pos1 + 1, pos2);
        if (_url.find("http://") == std::string::npos)
        {
            return BAD_REQUEST;
        }
        std::string method = text.substr(0, pos1);
        if (method == "GET")
        {
            _method = GET;
        }
        // 我们这里只支持GET
        else 
        {
            return BAD_REQUEST;
        }
        _version = text.substr(pos2 + 1);
        if(_version != "HTTP/1.1" && _version != "HTTP/1.0")
        {
            return BAD_REQUEST;
        }
        _check_state = CHECK_STATE_STATE_HEADER;
        return NO_REQUEST;
    }

    HTTP_CODE parse_request_headers(std::string &text)
    {
        // 如果读取到空行，则说明报头部分已经读取完毕
        if(text[0] == '\0')
        {
            // 如果HTTP请求有消息体，则还需读取_content_length字节的消息体，将状态转移到_CHECK_STATE_CONTEXT
            if(_content_length != 0)
            {
                _check_state = CHECK_STATE_CONTENT;
                return NO_REQUEST;
            }

            // 否则，就说明获取到了一个完整的HTTP请求
            return GET_REQUEST;
        }
        // 处理connection头部字段
        int pos = text.find("Connection:");
        if(pos != std::string::npos)
        {
            pos = text.find("keep-alive", pos);
            if(pos != std::string::npos)
            {
                _linger = true;
            }
        }
        // 处理Content_Length头部字段
        pos = text.find("Content-Length:", pos);
        if(pos != std::string::npos)
        {
            
        }
    }
    HTTP_CODE parse_request_content(char *text)
    {
    }
    HTTP_CODE do_request()
    {
    }
    char *get_line()
    {
        // return m_read_buf + m_start_line;
    }
    // 状态机
    LINE_STATUS parse_line()
    {
        char temp;
        //
        for (; _check_index < _read_index /*最后一个有效字符的下一个*/; ++_check_index)
        {
            // 获取当前要分析的字符
            temp = _read_buffer[_check_index];
            // 如果当前字符是'\r'，即回车符，则说明可能读到了一个完整的行
            if (temp == '\r')
            {
                // 如果'\r'碰巧是目前_read_buffer中的最后一个已经被读入的客户数据，那么此次
                // 分析没有读取到一个完整的行，返回LINE_OPEN以表示还需要继续读取客户数据才能进一步分析
                if ((_check_index + 1) == _read_index)
                {
                    return LINE_OPEN;
                }
                // 如果下一个字符是'\n'，则说明读取到一个完整的行
                else if (_read_buffer[_check_index + 1] == '\n')
                {
                    _read_buffer[_check_index++] = '\0';
                    _read_buffer[_check_index++] = '\0';
                    return LINE_OK;
                }
                // 否则的化给客户发送HTTP请求存在语法错误
                else
                {
                    return LINE_BAD;
                }
            }
            // 如果当前读到的字符为'\n'，即换行符，则也说明可能读取到一个完整的行
            else if (temp == '\n')
            {
                if (_check_index > 1 && _read_buffer[_check_index - 1] == '\r')
                {
                    _read_buffer[_check_index++] = '\0';
                    _read_buffer[_check_index++] = '\0';
                    return LINE_OK;
                }
                return LINE_BAD;
            }
        }
        // 如果当前的_read_buffer读取完还没有返回，说明没有读取完完整的行，还需要继续读取客户数据
        return LINE_OPEN;
    }

private:
    // 这一组函数用于被handel_write调用分析HTTP请求
    void unmap()
    {
    }
    bool add_response(const char *format, ...)
    {
    }
    bool add_content(const char *content)
    {
    }
    bool add_status_line(int status, const char *title)
    {
    }
    bool add_headers(int content_length)
    {
    }
    bool add_content_length(int content_legth)
    {
    }
    bool add_linger()
    {
    }
    bool add_blank_line()
    {
    }

public:
    // 所有socket上的事件都注册到同一个epoll模型中，所以将文件描述符设置为静态的
    static int _epollfd;
    // 统计用户数量
    static int _user_count;

private:
    // 该HTTP连接的socket和对方的socket的地址
    int _sockfd;
    // const struct sockaddr_in _peer;
    std::unordered_map<int, struct sockaddr_in> _peer_sockfd_addr;

    // 读缓存区
    char _read_buffer[READ_BUFFER_SIZE];
    // 标识该缓冲区客户已经读入的客户数据的最后一个字节的下一个位置
    int _read_index;
    // 当前正在分析的字符在读缓冲区中的位置
    int _check_index;
    // 当前正在解析的行的起始位置
    int _start_line;
    // 写缓冲区
    char _write_buffer[WRITE_BUFFER_SIZE];
    // 写缓冲区中待发送的字节数
    int _write_index;
    // 主状态机当前所处的状态
    CHECK_STATE _check_state;
    // 请求方法
    METHOD _method;
    // 客户请求的目标文件的完整路径，其内容等于doc_root + _url,doc_root是网站根目录
    char _real_file[FILENAME_LEN];
    // 客户请求的目标文件的文件名
    std::string _url;
    // HTTP版本协议号 仅支持HTTP/1.1
    std::string _version;
    // 主机名
    std::string _host;
    // HTTP请求的消息体长度
    int _content_length;
    // HTTP请求是否要保持连接
    bool _linger;
    // 客户请求的目标文件被mmap到内存中的起始位置
    char *_file_address;
    // 目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读...
    // struct stat _file_stat;
    // 我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量
    struct iovec m_iv[2];
    int _iv_count;
};

int http_conn::_user_count = 0;
int http_conn::_epollfd = -1;