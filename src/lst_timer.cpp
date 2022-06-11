#include"../include/lst_timer.h"
#include"../include/http_conn.h"
using namespace std;

void Utils::init(int timeslot) 
{ 
    m_TIMESLOT = timeslot; 
}
//将fd添加到epoll数组中，监听读事件
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if(1==TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    if(one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}
//自定义信号处理函数，创建sigaction结构体变量，设置信号函数。
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    //可重入性标识中断后再次进入该函数，环境变量与之前相同不会丢失数据
    int save_errno = errno;
    int msg = sig;

    //将信号从管道写端写入，传输字符类型，而非整型
    send(u_pipefd[1], (char *)&msg, 1, 0);

    //将原来的errno赋值为当前的errno
    errno = save_errno;
}
//注册信号函数(内部调用sigaction)
void Utils::addsig(int sig,void(handler)(int),bool restart=true)
{
    //创建并初始化结构体变量
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));//初始化为空字符

    //信号处理函数紧紧发送信号值 不做对应逻辑处理
    sa.sa_handler = handler;
    if(restart)
        sa.sa_flags |= SA_RESTART;//使被信号打断的系统调用自动重新发起
    sigfillset(&sa.sa_mask);      //将所有信号添加到信号集中
    int ret = sigaction(sig, &sa, NULL);
    assert(ret != -1);
}
//定时事件
void cb_func(client_data* user_data)
{
    //删除非活动连接在socket上的注册事件
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, NULL);
    assert(user_data);

    close(user_data->sockfd);  //关闭文件描述符
    http_conn::m_user_count--; //减少连接数
}

/*定时器容器类*/

//添加定时器
void sort_timer_list::add_timer(util_timer*timer,util_timer*lst_head)
{
    util_timer *prev_node = lst_head;
    util_timer *tmp = prev_node->next;

    //遍历当前结点之后的链表，按照超时时间找到目标定时器对应的位置，插入结点
    while(tmp)
    {
        if(timer->expire<tmp->expire)
        {
            prev_node->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev_node;
            break;
            }
        prev_node = tmp;
        tmp = tmp->next;
    }

    //如果定时器需要放到尾结点处
    if(!tmp)
    {
        prev_node->next = timer;
        timer->prev = prev_node;
        timer->next = NULL;
        tail = timer;
    }
}


void sort_timer_list::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}
void sort_timer_list::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

//定时任务处理函数
void sort_timer_list::tick()
{
    if(!head)
        return;

    time_t cur_time = time(NULL); //获取当前时间
    util_timer *tmp = head;

    //遍历定时器链表
    while(tmp)
    {
        //当前时间小于定时器的超时时间，后面的定时器也没有到期
        if (cur_time < tmp->expire)
            break;
        
        //当前定时器到期，则调用回调函数，执行定时事件
        tmp->cb_func(tmp->user_data);

        //处理后的定时器从链表删除 并重置头节点
        head = tmp->next;
        if(head)
            head->prev = NULL;
        delete tmp;
        tmp = head;
    }
}