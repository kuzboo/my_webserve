#include<string.h>
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
    //输出内容的长度
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', sizeof(m_buf));

    m_max_lines = max_lines;

    time_t t = time(NULL); //获取当前时间的秒数
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //从后往前找到第一个'/'的位置
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    //若输入的文件名没有/，则直接将时间+文件名作为日志名
    if (p == NULL)
    {
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", m_dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, m_log_name);
    }
    //若输入的文件名有/
    else
    {
        strcpy(m_log_name, p + 1);
        strncpy(m_dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", m_dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, m_log_name);
    }
    m_today = my_tm.tm_mday;

    m_fp = fopen(log_full_name, "a");
    if(m_fp==NULL)
        return false;
    return true;
}