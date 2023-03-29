#pragma once
#include "util.hpp"
#include <cstring>
#include <unordered_map>
#include <unistd.h>

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
        CHECK_STATE_CONTEXT
    };
    // 服务器处理HTTP请求的可能结果，状态码
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
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
        if(real_close &&( _sockfd != -1))
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
    HTTP_CODE parse_request_line(char *text)
    {
    }
    HTTP_CODE parse_request_headers(char *text)
    {
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
    LINE_STATUS parse_line()
    {
    }
private:
    // 这一组函数用于被handel_write调用分析HTTP请求
    void unmap()
    {}
    bool add_response(const char* format, ...)
    {}
    bool add_content(const char* content)
    {}
    bool add_status_line(int status, const char* title)
    {}
    bool add_headers(int content_length)
    {}
    bool add_content_length(int content_legth)
    {}
    bool add_linger()
    {}
    bool add_blank_line()
    {}
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
    char *_url;
    // HTTP版本协议号 仅支持HTTP/1.1
    char *_version;
    // 主机名
    char *_host;
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