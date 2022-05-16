#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include<unistd.h>
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
    };
public:
    http_conn();
    ~http_conn();

    bool read_once();//读取浏览器发来的所有请求
    bool write_once(); //响应报文写入

private:
    char m_read_buf[READ_BUFFER_SIZE];   //存储读取的请求报文数据
    char m_write_buf[WRITE_BUFFER_SIZE]; //存储发出响应报文的数据
    char m_real_file[FILENAME_LEN];      //读取文件的名称
};

bool http_conn::read_once()
{
    
}

#endif