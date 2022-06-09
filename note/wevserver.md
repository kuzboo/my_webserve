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