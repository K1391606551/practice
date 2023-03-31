#pragma once
#include "util.hpp"
#include <cstring>
#include <unordered_map>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>

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
        CHECK_STAT_HEADER,
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
        
        Util::Epoll::Addevent(_epollfd, sockfd, true);
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

    // 非阻塞写 ->写HTTP响应
    bool write()
    {
        int temp = 0;
        // 已经有多少字节发送了
        int bytes_have_send = 0;
        // 还有多少字节没有发送
        int bytes_to_send = _write_index;
        if(bytes_to_send == 0)
        {
            Util::Epoll::Modevent(_epollfd, _sockfd, EPOLLIN);
            init();
            return true;
        }

        while(true)
        {
            temp = writev(_sockfd, _iv, _iv_count);
            if(temp <= -1)
            {
                
            }
        }
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
    // 解析HTTP请求 主状态机
    HTTP_CODE handel_read()
    {
        LINE_STATUS line_status = LINE_OK;
        HTTP_CODE ret = NO_REQUEST;
        char *text = '\0';
        while (((_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) \ 
        || ((line_status = parse_line()) == LINE_OK))
        {
            text = get_line();
            _start_line = _check_index;
            std::cout << "get a http line: " << text << std::endl;
            switch (_check_state)
            {
                // 第一个状态，分析请求行
                case CHECK_STATE_REQUESTLINE:
                {
                    ret = parse_request_line(text);
                    if(ret == BAD_REQUEST)
                    {
                        return BAD_REQUEST;
                    }
                    break;
                }
                // 第二个状态，分析请求报头字段
                case CHECK_STAT_HEADER:
                {
                    ret = parse_request_headers(text);
                    if(ret == BAD_REQUEST)
                    {
                        return BAD_REQUEST;
                    }
                    else if(ret == GET_REQUEST)
                    {
                        return do_request();
                    }
                    break;
                }
                // 第三个状态，分析请求正文字段
                case CHECK_STATE_CONTENT:
                {
                    ret = parse_request_content(text);
                    if(ret == GET_REQUEST)
                    {
                        return do_request();
                    }
                    line_status = LINE_OPEN;
                    break;
                }
                default:
                {
                    return INTERNAL_ERROR;
                }
            }
        }

        return NO_REQUEST;
    }
    // 填充HTTP应答
    bool handel_write(HTTP_CODE ret)
    {
    }

private:
    // 这一组函数用于被handel_read调用分析HTTP请求
    HTTP_CODE parse_request_line(char *text)
    {
        // int pos1 = text.find(' ');
        // int pos2 = text.rfind(' ');
        // if (pos1 == pos2)
        // {
        //     return BAD_REQUEST;
        // }
        // _url = text.substr(pos1 + 1, pos2);
        // if (_url.find("http://") == std::string::npos)
        // {
        //     return BAD_REQUEST;
        // }
        // std::string method = text.substr(0, pos1);
        // if (method == "GET")
        // {
        //     _method = GET;
        // }
        // // 我们这里只支持GET
        // else
        // {
        //     return BAD_REQUEST;
        // }
        // _version = text.substr(pos2 + 1);
        // if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
        // {
        //     return BAD_REQUEST;
        // }
        // _check_state = CHECK_STATE_STATE_HEADER;
        // return NO_REQUEST;
    }

    HTTP_CODE parse_request_headers(char *text)
    {
        // 如果读取到空行，则说明报头部分已经读取完毕
        if (text[0] == '\0')
        {
            // 如果HTTP请求有消息体，则还需读取_content_length字节的消息体，将状态转移到_CHECK_STATE_CONTEXT
            if (_content_length != 0)
            {
                _check_state = CHECK_STATE_CONTENT;
                return NO_REQUEST;
            }

            // 否则，就说明获取到了一个完整的HTTP请求
            return GET_REQUEST;
        }
        // 处理connection头部字段
        // strncasecmp 忽略大小写比较前n个字符
        else if (strncasecmp(text, "Connection:", 11) == 0)
        {
            text += 11;
            // 返回第一个不在指定字符串出现的下标 example:
            // text->Connection: keep-alive,在text中第一不存在" "的下标为k的下标
            text += strspn(text, " "); // text指向k的地址
            if (strcasecmp(text, "keep-alive") == 0)
            {
                _linger = true;
            }
        }
        // 处理Content_Length头部字段
        else if (strncasecmp(text, "Content-Length:", 15) == 0)
        {
            text += 15;
            text += strspn(text, " ");
            _content_length = atoi(text);
        }
        // 处理Host头部字段
        else if (strncasecmp(text, "Host:", 5) == 0)
        {
            text += 5;
            text += strspn(text, " ");
            _host = text;
        }
        else
        {
            std::cout << "error! unkonw header : " << text << std::endl;
        }

        return NO_REQUEST;
    }
    // 我们没有正真解析消息体，只是判断它有没有读取完
    HTTP_CODE parse_request_content(char *text)
    {
        //
        if (_read_index >= (_content_length + _check_index))
        {
            text[_content_length] = '\0';
            return GET_REQUEST;
        }

        return NO_REQUEST;
    }
    // 得到一个完整的、正确的HTTP请求时，我们就分析目标文件属性，如果目标文件存在，对用户可读，且不是目录，
    // 则使用mmap将其映射到内存地址_file_address处，并告知调用者获得资源成功
    HTTP_CODE do_request()
    {
        strcpy(_real_file, doc_root);
        int len = strlen(doc_root);
        /*文件最大为FILENAME_LEN,已经拷贝进去len了，所有做多还可以拷贝FILENAME_LEN - len，留下一个字符肯有用*/
        strncpy(_real_file + len, _url, FILENAME_LEN - len - 1 );
        // 下去查看stat系统函数的作用
        if(stat(_real_file, &_file_stat) < 0)
        {
            return NO_REQUEST;
        }
        
        if(!(_file_stat.st_mode & S_IROTH))
        {
            return FORBIDDEN_REQUEST;
        }

        if(S_ISDIR(_file_stat.st_mode))
        {
            return BAD_REQUEST;
        }

        int fd = open(_real_file, O_RDONLY);
        _file_address = (char *)mmap(0, _file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        
        return FILE_REQUEST;   
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
    // 对内存映射区执行munmap操作
    void unmap()
    {
        if(_file_address)
        {
            munmap(_file_address, _file_stat.st_size);
            _file_address = 0;
        }
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
    char* _url;
    // HTTP版本协议号 仅支持HTTP/1.1
    char* _version;
    // 主机名
    char* _host;
    // HTTP请求的消息体长度
    int _content_length;
    // HTTP请求是否要保持连接
    bool _linger;
    // 客户请求的目标文件被mmap到内存中的起始位置
    char *_file_address;
    // 目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读...
    struct stat _file_stat;
    // 我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量
    struct iovec _iv[2];
    int _iv_count;
};

int http_conn::_user_count = 0;
int http_conn::_epollfd = -1;