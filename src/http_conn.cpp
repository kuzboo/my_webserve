#include"../include/http_conn.h"
using namespace std;

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
        if(bytes_read==-1) //recv返回值小于0 表示发生错误
        {
            /*
            非阻塞ET模式下，需要一次性将数据读完 
            这两种errno表示已经没有数据可读了直接跳出
            */
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return false;
        }
        else if(bytes_read==0)//返回0表示对端关闭连接
            return false;
        m_read_idx += bytes_read;
    }
    return true;
}

//套接字文件描述符设置成非阻塞
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return fd;
}

//向内核时间表注册新事件，开启EPOLLOENSHOT,针对客户端连接的描述符，listenfd不用开启
void addfd(int epollfd,int fd,bool one_shot)
{
    epoll_event event;  //声明一个epoll_event类型的变量
    event.data.fd = fd;//将需要监听的文件描述符 挂在到epoll结构体上
#ifdef ET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef LT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif
    if(one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//内核事件表删除事件  并关闭套接字
void removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//将事件重置为EPOLLONESHOT【最后两个参数什么意思】
void modfd(int epollfd,int fd,int ev,int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;
    if (1 == TRIGMode)//ET模式
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else//LT模式
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    //EPOLL_CTL_MOD更改注册的文件描述符的关注事件发生情况
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

//各子线程通过process函数对任务进行处理，完成报文解析与响应两个任务
void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if(read_ret==NO_REQUEST)
    {
        //modfd(m_epollfd, m_sockfd, EPOLLIN);注册读事件
        return;
    }

    bool write_ret = process_write(read_ret);
    if(!write_ret)
    {
        close_conn();
    }
    //modfd(m_epollfd, m_sockfd, EPOLLOUT);注册写事件
}

//从状态机解析一行数据 
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
http_conn::LINE_STATE http_conn::parse_line()
{
    char temp;
    for (; m_checked_idx < m_read_idx;++m_checked_idx)
    {
        temp = m_read_buf[m_checked_idx];
        if(temp=='\r')
        {
            if (m_checked_idx + 1 == m_read_idx) //是否到达了末尾
                return LINE_OPEN;
            else if(m_read_buf[m_checked_idx+1]=='\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                //checked_idx要移动到下一个位置
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            //【要+大于1】
            if(m_checked_idx>1 && m_read_buf[m_checked_idx-1]=='\r')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    //遍历完了没有找到\r \n 需要继续接收
    return LINE_OPEN;
}