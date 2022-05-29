#ifndef LST_TIMER
#define LST_TIMER

#include<unistd.h>
#include<signal.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<time.h>
#include<string.h>
#include<assert.h>




class Utils
{
public:
    Utils(){};
    ~Utils(){};

    void init(int timeslot);
    //信号处理函数
    static void sig_handler(int sig);
    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

public:
    static int *u_pipefd;
    static int u_epollfd;
    int m_TIMESLOT;
};

#endif