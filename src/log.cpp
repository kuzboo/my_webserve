#include"../include/log.h"

bool Log::init(const char *file_name, int log_buf_size, int max_lines, int max_queue_size)
{
    //如果设置了max_queue_size则设置异步
    if(max_queue_size>=1)
    {
        m_is_async = true;
        
        //创建并设置阻塞队列长度
        m_log_queue = new block_queue<string>(max_queue_size);
        pthread_t tid;

        //创建一个线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }
}