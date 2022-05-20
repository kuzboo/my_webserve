## http处理流程
- 浏览器端发出http连接请求，主线程创建http对象接收请求，并将所有数据读入对于buffer，然后将对象插入到任务队列，工作线程从任务队列中取出一个任务进行处理。
-
- 
## socket设置成非阻塞
int flg=fcntl(cfd,F_GETFL);
flg|=0_NONBLOCK;
fcntl(cfd,F_SETFL,flg);

## epoll
epoll两种工作模式
- ET 高效模式 只支持 非阻塞。


recv返回值小于0表示发生错误 设置errno 如果errno==EAGIN||EWOULDBLOCK 表示已经没有数据可读了

- EPOLLONESHOT
    一个线程读取某个socket上的数据后开始处理数据，在处理过程中该socket上又有新数据可读，此时另一个线程被唤醒读取，此时出现两个线程处理同一个socket

    我们期望的是一个socket连接在任一时刻都只被一个线程处理，通过epoll_ctl对该文件描述符注册epolloneshot事件，一个线程处理socket时，其他线程将无法处理，当该线程处理完后，需要通过epoll_ctl重置epolloneshot事件

- EPOLLRDHUP
在socket上接收到对方关闭连接的请求之后出发
- EPOLL_CTL_MOD：更改注册的文件描述符的关注事件发生情况