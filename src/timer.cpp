#include"../include/lst_timer.h"
using namespace std;

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

//设置信号函数
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

    assert(sigaction(sig, &sa, NULL) != -1);
}

//创建管道套接字
