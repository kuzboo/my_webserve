#ifndef LST_TIMER
#define LST_TIMER

#include<unistd.h>
#include<signal.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<errno.h>
#include<time.h>
#include<string.h>
#include<assert.h>

//定时器类
class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL){}
public:
    time_t expire;                  //超时时间
    void (*cb_func)(client_data *); //回调函数
    client_data *user_data;         //连接资源
    util_timer *prev;               //前向定时器
    util_timer *next;               //后向定时器
};

//连接资源结构体
struct client_data
{
    sockaddr_in clnt_addr;//客户端地址
    int sockfd;           //套接字文件描述符
    util_timer *timer;    //定时器类
};

//定时器容器类
class sort_timer_list
{
public:
    sort_timer_list() : head(NULL), tail(NULL){}
    ~sort_timer_list()
    {
        util_timer *s = head;
        while(s)
        {
            head = s->next;
            delete s;
            s = head;
        }
    }

    void add_timer(util_timer *timer);    //添加定时器，内部调用私有的add_timer;
    void adjust_timer(util_timer *timer); //调整定时器，任务发生变化时，调整位置
    void del_timer(util_timer *timer);    //删除定时器;
    void tick();                          //定时任务处理函数;

private:
    //私有成员，被add_timer和adjust_time调用
    void add_timer(util_timer *timer, util_timer *lst_head);
    util_timer *head;
    util_timer *tail;
};

//信号类
class Utils
{
public:
    Utils(){};
    ~Utils(){};

    void init(int timeslot);
    int setnonblocking(int fd);                                    //将文件描述符设置成非阻塞;
    static void sig_handler(int sig);                              //信号处理函数;
    void addsig(int sig, void(handler)(int), bool restart = true); //设置信号函数;
    void timer_handler();                                          //定时处理任务 重新定时以不断出发SIGALRM信号

    //向内核事件表注册读事件,ET模式,开启EPOLLONESHOT (触发trigger)
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd; 
    static int u_epollfd; 
    int m_TIMESLOT;
};
void cd_func(client_data *user_data); //定时事件删除非活动连接
#endif