#include<fstream>

#include"../include/http_conn.h"
using namespace std;

//响应状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

locker m_lock;
map<string, int> users;

//套接字文件描述符设置成非阻塞
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return fd;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~epoll相关~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//向内核时间表注册读事件，开启EPOLLOENSHOT,针对客户端连接的描述符，
//listenfd不用开启，TRIGmode=1标识边沿触发
void addfd(int epollfd,int fd,bool one_shot, int TRIGmode)
{
    epoll_event event;  //声明一个epoll_event类型的变量
    event.data.fd = fd;//将需要监听的文件描述符 挂在到epoll结构体上

    if(TRIGmode==1)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

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
//将事件重置为EPOLLONESHOT
void modfd(int epollfd,int fd,int ev,int TRIGmode)
{
    epoll_event event;
    event.data.fd = fd;

    if(TRIGmode==1)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

//静态成员类外初始化
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

//关闭一个连接
void http_conn::close_conn(bool real_close)
{
    if(real_close && (m_sockfd!=-1))
    {
        printf("close %d\n", m_sockfd);
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

//初始化连接，外部调用初始化套接字地址
void http_conn::init(int sockfd, const struct sockaddr_in &addr, char *root, int TRIGmode,
          int close_log,string user, string passwd, string sqlname)
{
    m_sockfd = sockfd;
    m_address = addr;

    addfd(m_epollfd, sockfd, true, m_TRIGmode);
    ++m_user_count;

    doc_root = root;
    m_TRIGmode = TRIGmode;
    m_close_log = close_log;

    strcpy(m_sql_user, user.c_str());
    strcpy(m_sql_name, sqlname.c_str());
    strcpy(m_sql_password, passwd.c_str());

    init(); //调用私有成员
}

//初始化接收新的连接 check_state默认为分析请求行状态
void http_conn::init()
{
    m_mysql = NULL;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_vesrion = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf,'\0',READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

//循环读取客户数据 读取到m_read_buf中并更新m_read_idx 直到无数据可读或对方关闭连接
bool http_conn::read_once()
{
    if(m_read_idx>=READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;

    if (0 == m_TRIGmode) //水平触发
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        
        if(bytes_read<=0)
            return false;

        m_read_idx += bytes_read;
        return true;
    }
    else//边沿触发
    {
        while(true)
        {
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if (bytes_read == -1) // recv返回值小于0 表示发生错误
            {
                /*
                非阻塞ET模式下，需要一次性将数据读完
                这两种errno表示已经没有数据可读了直接跳出
                */
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0) //返回0表示对端关闭连接
                return false;
            m_read_idx += bytes_read;
        }
        return true;
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~状态机~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
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
                m_read_buf[m_checked_idx-1] = '\0';
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
        //cgi=1;
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
        text += strspn(text, " \t");
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
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

//主状态机逻辑 解析内容
http_conn::HTTP_CODE http_conn::parser_content(char *text)
{
    //判断buffer中是否读了消息体
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        //POST请求中最后为输入的用户名和密码
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~process_read()~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//用于将指针后移 指向未处理的报文
char* http_conn::get_line()
{
    return m_read_buf + m_start_line;
}
//从m_read_buf读取 处理请求报文
http_conn::HTTP_CODE http_conn::process_read()
{
    //初始化从状态机状态 HTTP请求解析结果
    LINE_STATE line_state = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while (m_check_state == CHECK_STATE_CONTENT && line_state == LINE_OK 
                              || (line_state = parse_line()) == LINE_OK)
    {
        text = get_line();
        m_start_line = m_checked_idx;

        //主状态机的三种状态转移逻辑
        switch(m_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                //解析请求行
                ret = parse_request_line(text);
                if(ret==BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER:
            {
                //解析请求头
                ret = parsr_headers(text);
                if(ret=BAD_REQUEST)
                    return BAD_REQUEST;
                //完成GET请求解析后 跳转到报文响应函数
                else if(ret=GET_REQUEST)
                {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                //解析消息体
                ret = parser_content(text);
                //解析完消息体后跳转到报文响应函数
                if(ret==GET_REQUEST)
                    return do_request();
                //解析完消息体即完成报文解析 为了避免再次进入循环 更新line_state
                line_state = LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~do_request()~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
const char *doc_root = "/home/yhb/test/my_webServer/root"; //服务器根目录
//将网站根目录和url文件拼接，获取文件属性，将文件映射到内存
http_conn::HTTP_CODE http_conn::do_request()
{
    //将网站根目录赋值给m_real_file
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    printf("m_url:%s\n", m_url);
    //m_url最后一个/的位置
    const char *p = strrchr(m_url, '/');

    if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3'))
    {
        //根据标志判断是登录检测还是注册检测
        //同步线程登录校验
        //CGI多进程登录校验
    }
    //如果请求资源为/0 跳转到注册页面
    if (*(p + 1) == '0')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/register.html");
        //将网站目录和/register.html进行拼接，更新到m_real_file中
        /*
        【为什么要+len呢】m_real_file是这个字符串的首地址，如果不加len，就是从
        //首地址到strlen(m_url_real)这段地址内容赋值为m_url_real，所以+len为的是
        从最后一个地址开始赋值
        */
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        free(m_url_real);
    }
    //如果请求资源为/1 跳转到登陆界面
    else if (*(p + 1) == '1')
    {
        char *m_url_real = (char *)malloc(sizeof(char *) * 200);
        strcpy(m_url_real, "/log.html");
        //将网站目录与log合并 更新到m_real_file中
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        free(m_url_real);
    }
    //请求图片
    else if (*(p + 1) == '5')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    //请求video
    else if (*(p + 1) == '6')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '7')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/fans.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else
    {
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    }

    //通过stat获取请求资源文件信息
    //成功,将信息更新到m_file_stat结构体中;失败 返回NO_RESOURCE标识资源不存在
    if(stat(m_real_file,&m_file_stat)<0)
        return NO_RESOURCE;
    //判断文件的权限，是否可读，不可读则返回FORBIDDEN_REQUEST状态
    if(!(m_file_stat.st_mode&S_IROTH))
        return FORBIDDEN_REQUEST;
    //判断文件类型，如果是目录，则返回BAD_REQUEST，表示请求报文有误
    if(S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;

    //以只读方式获取文件描述符，通过mmap将该文件映射到内存中
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char *)mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    //关闭文件描述符 避免浪费和占用
    close(fd);

    //标识请求文件存在 且可以访问
    return FILE_REQUEST;
}
//取消内存映射
void http_conn::unmap()
{
    if(m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~write()~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//将写缓冲区的数据发送到浏览器端
bool http_conn::write()
{
    int temp = 0;
    int newadd = 0;
    
    //如果要发送字节数据为空 标识响应报文为空 重新注册读事件
    if(!bytes_to_send)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN,m_TRIGmode);
        init();
        return true;
    }

    while(1)
    {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if(temp>0) //如果正常发送
        {
            bytes_have_send += temp;//更新已经发送字节
            newadd = bytes_have_send - m_write_idx;//偏移文件iovec的指针？？？？
        }
        if(temp<=-1)
        {
            if(errno==EAGAIN)//buf不可写 缓冲区满了
            {
                //第一个iovec发送完毕，发送第二个iovec数据
                //意思就是头部发送完毕 只需要发送文件数据
                if (bytes_have_send >= m_iv[0].iov_len)
                {
                    //不再继续发送头部信息
                    m_iv[0].iov_len = 0;
                    //这两句什么意思？？？
                    m_iv[1].iov_base = m_file_address + newadd;
                    m_iv[1].iov_len = bytes_to_send;
                }
                else//头部也没有发送成功 继续发送头部
                {
                    m_iv[0].iov_base = m_write_buf + bytes_to_send;
                    m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
                }
                //重新注册写事件
                modfd(m_epollfd, m_sockfd, EPOLLOUT,m_TRIGmode);
                return true;
            } 
            //如果发送失败  不是缓冲区问题 取消映射
            unmap();
            return false;
        }
        //更新已发送字节数
        bytes_to_send -= temp;
        //如果数据全部发送完
        if(bytes_to_send<=0)
        {
            unmap();
            //在epoll树上重置EPOLLONESHOT事件
            modfd(m_epollfd, m_sockfd, EPOLLIN,m_TRIGmode);

            //浏览器的请求为长连接
            if(m_linger)
            {
                //重新初始化HTTP对象
                init();
                return true;
            }
            else
                return false;
        }
    }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~add_response()~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*将可变参列表加入到写缓冲区，更新 m_write_idx*/
bool http_conn::add_response(const char *format...)
{
    if(m_write_idx>=WRITE_BUFFER_SIZE)
        return false;

    va_list arg_list;           //定义可变参数列表
    va_start(arg_list, format); //将变量arg_list初始化为传入参数
    //将数据format从可变参数列表写入缓冲区写，返回写入数据的长度。因为包括\0所以第二个参数size要-1
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    //如果写入的数据长度超过缓冲区剩余空间，则报错
    if(len>=(WRITE_BUFFER_SIZE-m_write_idx-1))
    {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;//更新写缓冲区位置
    va_end(arg_list);  //清空可变参列表
    return true;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~添加响应报文内部调用add_response()~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//添加状态行
bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
//添加响应报文长度
bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}
//添加文本类型这里是html
bool http_conn::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}
//添加连接状态
bool http_conn::add_linger()
{
    return add_response("Connection:%s\r\n", 
    (m_linger == true) ? "keep-alive" : "close");
}
//添加空行
bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}
//添加消息头(文本长度 连接状态 空行)
bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() && add_blank_line();
}
//添加文本
bool http_conn::add_content(const char* content)
{
    return add_response("%s", content);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~process_write()~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*
根据process_read对请求报文的解析结果，产生对应的响应报文(包括状态行消息头消息体)
通过iovec结构体指定响应报文的地址和长度，状态为OK的情况需要申请两个iovec，一个指向
响应报文一个指向要传输的文件，而其余状态只申请一个就行，指向响应报文
*/
bool http_conn::process_write(HTTP_CODE ret)
{
    switch(ret)
    {
        //服务器内部错误，500
        case INTERNAL_ERROR:
        {
            add_status_line(500, "error_500_title");//状态行
            add_headers(strlen("error_500_form"));  //消息头
            if (!add_content("error_500_form"))     //消息体
                return false;
            break;
        }
        //客户对资源没有足够的访问权限，403
        case FORBIDDEN_REQUEST:
        {
            add_status_line(500, "error_403_title");//状态行
            add_headers(strlen("error_403_form"));  //消息头
            if (!add_content("error_403_form"))     //消息体
                return false;
            break;
        }
        //报文语法有误， 400
        case BAD_REQUEST:
        {
            add_status_line(400, error_400_title);//状态行
            add_headers(strlen(error_400_form));  //消息头
            if (!add_content(error_400_form))     //消息体
                return false;
            break;
        }
        //无法找到资源，404
        case NOT_FOUND:
        {
            add_status_line(400, error_404_title);//状态行
            add_headers(strlen(error_404_form));  //消息头
            if (!add_content(error_404_form))     //消息体
                return false;
            break;
        }
        //文件存在，200
        case FILE_REQUEST:
        {
            add_status_line(200, ok_200_title);
            //如果请求的资源存在
            if (m_file_stat.st_size != 0)
            {
                add_headers(m_file_stat.st_size);
                //第一个iovec指针指向响应报文缓冲区，长度指向m_write_idx 
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                //第二个指向mmap返回的文件指针，长度指向文件大小
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                //发送的全部数据为响应报文头部信息和文件大小
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                return true;
            }
            else
            {
                //如果请求资源大小为0 返回空html文件
                const char *ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string))
                    return false;
            }
        }
        default:
            return false;
    }
    //除了FILE_REQUEST状态外，其余状态只申请一个iovec，指向响应报文缓冲区
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

//各子线程通过process函数对任务进行处理，完成报文解析与响应两个任务
void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if(read_ret==NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGmode);//注册读事件
        return;
    }

    bool write_ret = process_write(read_ret);
    if(!write_ret)
    {
        close_conn();
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT,m_TRIGmode);//注册写事件
}


//将数据库中的用户名和密码载入到服务器的map中来，map中的key为用户名，value为密码。
void http_conn::initMySQL_result(connection_pool *connPool)
{
    MYSQL *mysql = NULL;
    connectionRAII(&mysql, connPool);//从连接池获取一个连接

    //在user表中检索username passwd数据，浏览器出入
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }
    
    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
    int num_fileds = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fileds = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]); //用户名
        string temp2(row[1]); //密码
        m_users[temp1] = temp2;
    }
}
