## http处理流程
- 浏览器端发出http连接请求，主线程创建http对象接收请求，并将所有数据读入对于buffer，然后将对象插入到任务队列，工作线程从任务队列中取出一个任务进行处理。
-工作线程取出任务后，调用process_read函数，通过主、从状态机对请求报文进行解析
-解析完之后，跳转do_request函数生成响应报文，通过process_write写入buffer，返回给浏览器端

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

## 解析报文整体流程

### process_read函数
通过while循环，将主从状态机进行封装，对报文的每一行进行循环处理
- 判断条件
  - 主状态机转移到CHECK_STATE_CONTENT，该条件涉及解析消息体
  - 从状态机转移到LINE_OK，该条件涉及解析请求行和请求头部
  - 两者为或关系，当条件为真则继续循环，否则退出

  - 为什么要写成
  `while((m_check_state==CHECK_STATE_CONTENT && line_status==LINE_OK)||((line_status=parse_line())==LINE_OK))`
    在GET请求报文中，每一行都是\r\n作为结束，所以对报文进行拆解时，仅用从状态机的状态
    `line_status=parse_line()==LINE_OK`语句即可。但，在POST请求报文中，消息体的末尾
    没有任何字符，所以不能使用从状态机的状态，这里转而使用主状态机的状态作为循环入口条件。
  - 那后面的&& line_status==LINE_OK又是为什么?
    解析完消息体后，报文的完整解析就完成了，但此时主状态机的状态还是CHECK_STATE_CONTENT，也就是说，符合循环入口条件，还会再次进入循环，这并不是我们所希望的。为此，增加了该语句，并在完成消息体解析后，将line_status变量更改为LINE_OPEN，此时可以跳出循环，完成报文解析任务。

- 循环体
  - 从状态机读取数据
  - 调用get_line函数 通过m_start_line将从状态机数据间接赋值给text
  - 主状态机解析text

  

### 主状态机逻辑
主状态机初始状态是CHECK_STATE_REQUESTLINE，通过调用从状态机来驱动主状态机，在主状态机进行解析前，从状态机已经将每一行的末尾\r\n符号改为\0\0，以便于主状态机直接取出对应字符串进行处理。
- CHECK_STATE_REQUESTLINE
  - 主状态机的初始状态，调用parse_request_line函数解析请求行
  - 解析函数从m_read_buf中解析HTTP请求行，获得请求方法、目标URL及HTTP版本号
  - 解析完成后主状态机的状态变为CHECK_STATE_HEADER
- CHECK_STATE_HEADER
  - 调用parse_headers函数解析请求头部信息
  - 判断是空行还是请求头，若是空行，进而判断content-length是否为0，如果不是0，表明是POST请求，则状态转移到CHECK_STATE_CONTENT，否则说明是GET请求，则报文解析结束。
  - 若解析的是请求头部字段，则主要分析connection字段，content-length字段，其他字段可以直接跳过
  - connection字段判断是keep-alive还是close，决定是长连接还是短连接
  - content-length字段，这里用于读取post请求的消息体长度
- CHECK_STATE_CONTENT
  - 仅用于解析POST请求，调用parse_content函数解析消息体
  - 用于保存post请求消息体，为后面的登录和注册做准备

### 从状态机逻辑
HTTP报文每一行的数据由\r\n结束，空行则是仅仅是字符\r\n。因此，可以通过查找\r\n将报文拆解成单独的行进行解析。从状态机负责读取buffer中的数据，将每行数据末尾的\r\n置为\0\0，并更新从状态机在buffer中读取的位置m_checked_idx，以此来驱动主状态机解析。
- 从状态机从m_read_buf中逐字读取 判断是否为\r
  - 如果接下来的是\n 将\r\n修改为\0\0 将m_checked_idx指向下一行开头 返回LINE_OK
  - 接下来达到了buffer末尾，表示buffer还需要继续接收，返回LINE_OPEN
  - 否则 语法错误返回LINE_BAD
- 当前字节不是\r 判断是不是\n(一般是上次读到了\r就到了buffer末尾 这次需要继续接收)
  - 如果前一个字符是\r 将\r\n修改为\0\0 将m_checked_idx指向下一行开头 返回LINE_OK
- 当前既不是\r也不是\n 接收不完整继续接收 返回LINE_OPEN