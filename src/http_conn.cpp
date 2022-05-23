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

//主状态机逻辑 解析请求行，获得请求方法，目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    //GET http:/somedir/page.html HTTP/1.1
    m_url = strpbrk(text, " \t"); // url=" http://somedir/page.html HTTP/1.1"
    if (!m_url)
        return BAD_REQUEST;
    *m_url++ = '\0';     // 去掉前面的空格url="http://somedir/page.html HTTP/1.1"
                         // 同时 text="GET\0"
    char *method = text; // method="GET"
    if(strcasecmp(method,"GET")==0)
        m_method = GET;
    else if(method,"POST"==0)
    {
        m_method = POST;
        // cgi=1;
    }
    else
        return BAD_REQUEST;
    
    //m_url此时跳过了第一个空格或\t字符，但不知道之后是否还有
    //将m_url向后偏移，通过查找，继续跳过空格和\t字符，指向请求资源的第一个字符
    //假如说第一个/之前有好几个空格 可以定位到空格后的第一个字符
    m_url += strspn(m_url, " \t");
    m_vesrion = strpbrk(m_url, " \t"); // version=" HTTP/1.1";
    if (!m_vesrion)
        return BAD_REQUEST;
    *m_vesrion++ = '\0';               //去掉空格version="HTTP/1.1"
                                       //url="http://somedir/page.html"
    m_vesrion += strspn(m_vesrion, " \t");
    if(!strcasecmp(m_vesrion,"HTTP/1.1")!=0)
        return BAD_REQUEST;
    
    //增加http://情况
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    //增加https://情况
    if (strncasecmp(m_url, "https://", 8) == 0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');//第一次出现/的位置
    }
    //一般的不会带有上述两种符号，直接是单独的/或/后面带访问资源
    if(!m_url || m_url[0]!='/')
        return BAD_REQUEST;
    
    //当url为/时 标识欢迎界面
    if(strlen(m_url)==1)
        strcat(m_url, "judge.html");
    
    //请求行处理完毕 主状态机转移处理请求头
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//主状态机逻辑 解析请求头
http_conn::HTTP_CODE http_conn::parsr_headers(char *text)
{
    //判断空行还是请求头
    if(text[0]=='\0')
    {
        if(m_content_length!=0)//如果不为0说明是post请求
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;//如果为0说明是get请求 请求结束
    }
    //解析头部 连接字段
    else if(strncasecmp(text,"Connection:",11)==0)
    {
        text += 11;
        if(strcasecmp(text,"keep-alive")==0)//如果是长连接
        {
            m_linger = true;
        }
    }
    //解析头部 内容长度字段
    else if(strncasecmp(text,"Content-length:",15)==0)
    {
        text += 15;
        text+=strspn(text," \t");
        m_content_length=atol(text);
    }
    //解析请求头部HOST字段
    else if(strncasecmp(text,"Host:",5)==0)
    {
        text+=5;
        text+=strspn(text," \t");
        m_host=text;
    }
    else
    {
        printf("oop!unknow header: %s\n",text);
    }
    return NO_REQUEST;
}