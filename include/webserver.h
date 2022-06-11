#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<stdio.h>

#include"../include/http_conn.h"
#include"../include/threadpool.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时时间

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void init(int port, string user, string passWord, string databaseName,
              int log_write, int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);

    void trig_mode();
    void log_write();
    void sql_pool();
    void thread_pool();
    void eventlisten();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclientdata();
    bool dealwithsignal(bool &timeout, bool &stop_server);
    void dealwith_read(int sockfd);
    void dealwith_write(int sockfd);
    void eventLoop();

public:
    int m_port;      //主机端口号
    char *m_root;    //网站根目录
    int m_log_write; //1代表异步写
    int m_close_log; //日志开关
    int m_actormodel; // 1代表reactor

    int m_pipefd[2];
    int m_epollfd;
    http_conn *users; //代表用户请求

    //数据库相关
    connection_pool *m_connPool;
    string m_user;         //登录数据库用户名
    string m_passWord;     //登录数据库密码
    string m_databaseName; //使用的数据库名
    int m_sql_num;
    
    //线程池相关
    threadpool<http_conn> *m_pool;//【为什么这样定义】
    int m_thread_num;

    //epoll相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER; //0close()立即返回 1不会
    int m_TRIGMode;
    int m_LISTENTrigmode;
    int m_CONNTrigmode;

    //定时器相关
    client_data *users_timer; //连接资源;
    Utils utils;
};

#endif