#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/uio.h>
#include<string.h>
#include<errno.h>
#include<mysql/mysql.h>
#include<string>
#include<map>

#include"locker.h"
#include"lst_timer.h"
#include"connection_pool.h"
#include"log.h"

using namespace std;

class http_conn
{
public:
    static const int FILENAME_LEN = 200;       //文件名称大小
    static const int READ_BUFFER_SIZE = 2048;  //读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024; //写缓冲区大小
    enum METHDO
    {
        GET = 0,
        POST
    }; //报文请求方法
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    }; //主状态机标识解析位置(解析请求行，解析头部，解析消息体)
    enum LINE_STATE
    {
        LINE_OK,  //完整读取一行
        LINE_BAD, //报文语法有误
        LINE_OPEN //读取的行不完整
    };            //从状态机,标识解析一行的读取状态
    enum HTTP_CODE
    {
        NO_REQUEST = 0,    //表示请求不完整，需要继续接收请求数据
        GET_REQUEST,       //获得了完整的HTTP请求
        BAD_REQUEST,       //HTTP请求报文有语法错误
        NO_RESOURCE,       //请求不完整需要继续获取请求
        FORBIDDEN_REQUEST, //客户对资源没有足够的访问权限
        NOT_FOUND,         //服务器无法根据客户端的请求找到资源
        FILE_REQUEST,      //文件存在
        INTERNAL_ERROR     //服务器内部错误500 该结果在主状态机逻辑switch的default下，一般不会触发;
    };                     //报文解析的结果

public:
    http_conn();
    ~http_conn();
    //初始化套接字地址，函数内部会调用私有方法init
    void init(int sockfd, const struct sockaddr_in &addr, char *, int, int,
              string user, string password, string sqlname);
    void close_conn(bool read_close = true); //关闭http连接
    bool read_once();                        //一次性读取浏览器发来的所有请求
    bool write();                            //响应报文发送到服务器套接字缓冲区
    void process();                          //各子线程通过process函数对任务进行处理，完成报文解析与响应两个任务;
    sockaddr_in *get_address() { return &m_address; }
    void initMySQL_result(connection_pool *connPool);
    
    static int m_epollfd;    //
    static int m_user_count; //
    int timer_flag;
    int improv;//1代表该请求还未处理完毕
    int m_state;//读为0，写为1
    MYSQL *m_mysql;

private:
    void init();                              //
    HTTP_CODE process_read();                 //从m_read_buf读取 处理请求报文
    bool process_write(HTTP_CODE ret);        //向m_write_buf写入响应报文数据
    HTTP_CODE parse_request_line(char *text); //主状态机解析 行数据
    HTTP_CODE parsr_headers(char *text);      //主状态机解析 头数据
    HTTP_CODE parser_content(char *text);     //主状态机解析 内容
    LINE_STATE parse_line();                  //从状态机解析一行数据 分析是报文的那一部分
    HTTP_CODE do_request();                   //生成响应报文
    char *get_line();                         // get_line用于将指针向后偏移，指向未处理的字符

    void unmap(); //取消内存映射

    bool add_response(const char *format...);
    bool add_status_line(int status, const char *title); //添加状态行
    bool add_headers(int content_len);//添加消息头
    bool add_content_type();
    bool add_content_length(int content_len);
    bool add_linger(); //添加连接状态
    bool add_blank_line();//添加空行
    bool add_content(const char *content); //添加文本

    int m_sockfd;
    sockaddr_in m_address;               //套接字地址
    char m_read_buf[READ_BUFFER_SIZE];   //存储读取的请求报文数据
    char m_write_buf[WRITE_BUFFER_SIZE]; //存储发出响应报文的数据
    char m_real_file[FILENAME_LEN];      //读取文件的名称
    int m_read_idx;                      // m_read_buf中数据的最后一个字节的下一个位置
    int m_write_idx;                     //m_write_buf中最后一个字节的下一个位置
    int m_checked_idx;                   // m_read_buf中读取的位置
    int m_start_line;                    // m_read_buf已经解析的字符的个数
                                            
    METHDO m_method;                     //请求方法
    CHECK_STATE m_check_state;           //主状态机状态

    //请求报文中对应的变量
    char m_real_file[FILENAME_LEN];//存储读取文件的名称
    char *m_url;
    char *m_vesrion;
    char *m_host;
    int m_content_length;
    bool m_linger; // true为长连接

    int cgi;                 //是否启用cgi
    char *m_string;          //存储请求头数据
    struct stat m_file_stat; //文件属性
    struct iovec m_iv[2];    //io向量机制iovec
    int m_iv_count;          // iovec结构体数组长度
    char *m_file_address;    //读取服务器上文件地址
    int bytes_to_send;       //剩余发送字节数
    int bytes_have_send;     //已发送字节数

    map<string, string> m_users; //用户名和密码
    int m_TRIGmode;              // epoll触发模式
    int m_close_log;

    char m_sql_user[100]; //数据库用户名
    char m_sql_password[100];//登录密码
    char m_sql_name[100];//登录用户名
};

#endif