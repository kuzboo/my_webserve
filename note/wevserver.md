WebServer()
- 初始化MAX_FDhttp_conn对象用于连接
- 获取当前工作工作目录的绝对路径，然后再后面加上/root
- 将每个连接资源进行封装，包含客户端地址、用户连接的套接字文件描述符和定时器

void init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
初始化 端口号、登录数据库用户名、密码、数据库名、写日志方式(1代表异步)、长/短连接、触发模式、数据库连接数量、线程数、日志是否关闭、反应模式

void trig_mode()
触发模式，具体的包含监听对象的触发模式和连接触发模式，LT+ET组合，一共有四种

void log_write()
通过Log::getinstance获取日志实例,然后根据log_write是1还是0初始化日志文件，缓冲区大小，最大行数，最长日志条队列(若是同步则为0)

void sql_pool()
获取数据库连接池实例，初始化连接池，具体的包括服务器名、数据库用户名、密码、数据库名、端口号、连接数量、日志开关

void thread_pool()
new一个线程池对象

void timer(int connfd, struct sockaddr_in client_address)
- 初始化连接，将connfd挂载到epoll上，监听读事件
- 初始化client_data数据
- 设置一个定时器，包括回调函数和超时时间，与客户数据绑定在一起，添加到定时器容器中

## void eventlisten()
- 创建用于监听套接字文件描述符 socket()
- 根据m_OPT_LINGER参数设置套接字的状态，通过SO_LINGER选项设置延迟关闭时间，具体的通过linger结构体和setsockopt()函数
- 为套接字绑定IP地址和端口号，bind()之前将套接字设置成SO_REUSEADDR状态，端口复用
- 通过listen()设置内核监听队列最大长度
- 为信号处理设置超时时间
- epoll创建内核事件表，将listenfd挂载到epoll上，监听读事件
- 通过setsockpair()创建全双工的管道套接字,挂载到epoll上监听读事件

## void eventLoop()
初始化两个标志 bool timeout = false, stop_server = false;
当stop_server=false 循环调用epoll_wait阻塞监听事件，如果返回值大于0，说明有事件发生，for循环遍历epoll数组中每一项的文件描述符和事件
- 如果是listenfd，说明其上发生了读事件 有新连接发生，执行dealclientdata(),通过accept创建一个连接，然后为该连接启动一个定时器。
- 如果发生的事件是EPOLLRDHUP | EPOLLHUP | EPOLLERR，则关闭连接删除定时器
- 如果发生事件的文件描述符是管道读端而且事件是读事件，说明有信号触发，执行dealwithsignal(),这个函数有两个参数分别是timeout和stop_server，bool类型的引用，函数内部通过recv读取数据，如果收到的信号是SIGALRM，说明触发超时，令timeout=true，；如果信号时SIGTERM，则令stop_sever=true，关闭服务器。
- 如果事件是读事件，执行dealwith_read();
- 如果事件是些事件，执行dealwiht_write();
