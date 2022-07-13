## include 头文件 
## src 源文件 
## note 笔记
## test 测试cmake
## practice 常见问题的源代码练习


## http模块
浏览器端发送http连接请求，服务端主线程创建http对象接收请求，并将所有数据一次性读入对应buffer，
将该对象插入到任务队列，然后工作线程从队列中取出一个任务进行处理。各个子线程调用process()
函数对任务进行处理，内部分别调用process_read() process_write()分别完成报文的解析和响应

状态机逻辑
while (m_check_state == CHECK_STATE_CONTENT && line_state == LINE_OK 
                              || (line_state = parse_line()) == LINE_OK)
状态机循环入口
从状态机负责读取报文的一行，主状态机负责对该行数据进行解析
具体的，初始化主状态机为CHECK_STATE_REQUESTLINE，从状态机为LINE_OK，次状态调用解析请求头函数，如果获得完整一行，则主状态机转为CHECK_STATE_HEADER，调用解析请求头函数，通过判断内容字段是否为0，如果为0表示get请求，解析结果为GET_REQUEST，状态机结束；如果不为0为post请求，主状态机转到CHECK_STATE_CONTENT，调用解析消息体函数，完了之后从状态机状态要为LINE_OPEN防止再次进入循环。

process_write()
根据process_read对请求报文的解析结果，产生对应的响应报文(包括状态行消息头消息体)
通过iovec结构体指定响应报文的地址和长度，状态为OK的情况需要申请两个iovec，一个指向
响应报文一个指向要传输的文件，而其余状态只申请一个就行，指向响应报文