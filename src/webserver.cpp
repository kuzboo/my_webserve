#include"../include/webserver.h"
using namespace std;

WebServer::WebServer()
{
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);//获取当前工作目录的绝对路径
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcpy(m_root, root);

    //定时器
    users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}

void WebServer::init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

void WebServer::trig_mode()
{
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::log_write()
{
    if(0==m_close_log)
    {
        if(1==m_log_write)//异步写
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 80000, 800);
        else
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 80000);
    }
}

void WebServer::sql_pool()
{
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log);

    //初始化数据库读取表
    users->initMySQL_result(m_connPool);
}

void WebServer::thread_pool()
{
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}

void WebServer::eventlisten()
{
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    //优雅的关闭连接
    if(0==m_OPT_LINGER)
    {
        struct linger tmp = {0, 1}; //执行close()会立即返回，但底层会将未发送完的数据发送完后再释放
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, (void *)&tmp, sizeof(tmp));
    }
    else if(1==m_OPT_LINGER)
    {
        struct linger tmp = {1, 1};//设置超时1s内发送完所有数据close()返回正确否则错误
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, (void *)&tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));//设置端口复用
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);//监听m_listenfd读事件
    http_conn::m_epollfd = m_epollfd;

    //创建管道套接字(全双工) pipe(是半双工)
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]); //写端非阻塞
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);//监听管道读端读事件

    utils.addsig(SIGPIPE, SIG_IGN); //SIG_IGN作用 忽略SIGPIPE信号，因为其会杀死进程
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);

    alarm(TIMESLOT);
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

//为一条连接创建定时器
void WebServer::timer(int connfd, struct sockaddr_in client_address)
{
    users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode,
                       m_close_log, m_user, m_passWord, m_databaseName);
    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].clnt_addr = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;//回调函数[为什么不是静态]

    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;//超时时间
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}

//删除定时器
void WebServer::deal_timer(util_timer *timer,int sockfd)
{
    timer->cb_func(&users_timer[sockfd]);
    if(timer)
    {
        utils.m_timer_lst.del_timer(timer);
    }

    LOG_INFO("close fd &d", users_timer[sockfd].sockfd);
}

//创建一个连接，然后为该连接启动定时器
bool WebServer::dealclientdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if(0==m_LISTENTrigmode)
    {
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if(connfd<0)
        {
            LOG_ERROR("%s:errno is %d", "accept error", errno);
            return false;
        }
        if(http_conn::m_user_count>=MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);
    }
    else
    {
        while(1)
        {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD)
            {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

//处理管道读端中的信号 判断是否超时 是否停止服务
bool WebServer::dealwithsignal(bool &timeout,bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if(ret==-1)
    {
        return false;
    }
    else if(ret==0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch(signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void WebServer::dealwith_read(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    //reator
    if(1==m_actormodel)
    {
        //如果有事件发生 调整定时器
        if(timer)
            adjust_timer(timer);
        
        //若检测到读事件 将事件放入请求队列中
        m_pool->append(users + sockfd, 0);//users 是一个http_conn数组头指针，偏移sockfd代表第几个请求

        while(true)
        {
            if(1==users[sockfd].improv)
            {
                if(1==users[sockfd].timer_flag)
                {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    //proactor
    else
    {
        if(users[sockfd].read_once())
        {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            //若检测到读事件 放入请求队列
            m_pool->append_p(users + sockfd);
            if(timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            deal_timer(timer, sockfd);
        }
    }
}

void WebServer::dealwith_write(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    //reactor
    if(1==m_actormodel)
    {
        //如果有事件发生 调整定时器
        if(timer)
        {
            adjust_timer(timer);
        }
        m_pool->append(users + sockfd, 1);

        while(true)
        {
            if(1==users[sockfd].improv)
            {
                if(1==users[sockfd].timer_flag)
                {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    //proactor
    else
    {
        if(users[sockfd].write())
        {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            if(timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            deal_timer(timer, sockfd);
        }
    }
}

void WebServer::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while(!stop_server)
    {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if(number<0 && errno!=EINTR) //EINTR？？
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number;++i)
        {
            int sockfd = events[i].data.fd; //events传出的是有事件发生的文件描述符

            //处理新的客户连接
            if(sockfd==m_listenfd)  //如果m_listenfd上有读时间  说明有新的连接
            {
                bool flag = dealclientdata();
                if(false==flag)
                {
                    continue;
                }
            }
            //服务器关闭连接，移除对应的定时器
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //处理信号 处理管道读事件
            else if((sockfd==m_pipefd[0]) && events[i].events & EPOLLIN)
            {
                bool flag = dealwithsignal(timeout, stop_server);
                if(flag==false)
                {
                    LOG_ERROR("%s", "deal client data failure");
                }
            }
            //处理客户连接上读事件
            else if (events[i].events & EPOLLIN)
            {
                dealwith_read(sockfd);
            }
            //处理客户写事件
            else if(events[i].events & EPOLLOUT) //什么时候注册的写事件呢
            {
                dealwith_write(sockfd);
            }
        }
        if(timeout)
        {
            utils.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}