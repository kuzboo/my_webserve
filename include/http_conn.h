#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<string.h>

#include"locker.h"

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
    }; //主状态机状态(解析请求行，解析头部，解析消息体)
    enum LINE_STATE
    {
        LINE_OK,
        LINE_BAD,
        LINE_OPEN
    }; //从状态机的状态
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST
    }; //报文解析的结果

public:
    http_conn();
    ~http_conn();

    bool read_once();  //读取浏览器发来的所有请求c
    bool write_once(); //响应报文写入
private:
    HTTP_CODE parse_request_line(char *text); //主状态机解析 行数据
    HTTP_CODE parsr_headers(char *text);      //主状态机解析 头数据
    HTTP_CODE parser_content(char *text);     //主状态机解析 内容
    LINE_STATE parse_line();                  //从状态机解析一行数据 分析是报文的那一部分

private:
    int m_sockfd;
    char m_read_buf[READ_BUFFER_SIZE];   //存储读取的请求报文数据
    char m_real_file[FILENAME_LEN];      //读取文件的名称
    int m_read_idx;                      //m_read_buf中数据的最后一个字节的下一个位置;
    int m_start_line;                    // m_read_buf已经解析的字符的个数

    char m_write_buf[WRITE_BUFFER_SIZE]; //存储发出响应报文的数据
    int m_write_idx;                     //写缓冲区的位置

    METHDO m_method; //请求方法
};

//循环读取客户数据 读取到m_read_buf中并更新m_read_idx 直到无数据可读或对方关闭连接 
bool http_conn::read_once()
{
    if(m_read_idx>=READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;
    while(true)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if(bytes_read==-1)
        {
            
        }
    }
}

#endif