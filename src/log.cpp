#include<string.h>
#include<sys/time.h>
#include<stdarg.h>
#include"../include/log.h"

/*
服务器启动按当前时刻创建日志，前缀为时间，后缀为自定义log文件名
并记录创建日志的时间day和行数count
*/
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

    //获取当前时间
    time_t t = time(NULL); 
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //从后往前找到第一个'/'的位置
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};//日志文件全名 时间+自定义log文件名

    //若输入的文件名没有'/'，则直接将时间+文件名作为日志名
    if (p == NULL)
    {
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", m_dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, m_log_name);
    }
    //若输入的文件名有'/'
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

//将系统信息格式化后输出，具体为：格式化时间 + 格式化内容
void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t now_time = now.tv_sec;
    struct tm *sys_tm = localtime(&now_time);
    struct tm my_tm = *sys_tm;

    char s[16] = {0};

    //日志分级
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    m_mutex.lock();

    m_count++; //更新现有行数
    //日志不是今天或写入的日志行数是最大行的倍数
    //m_count从0开始增加 取模为0说明相等
    if (m_today != my_tm.tm_mday || m_count % m_max_lines == 0)
    {
        char new_log[256] = {0};
        fflush(m_fp);//强迫将缓冲区内的数据写回m_fp中
        fclose(m_fp);
        char tail[16] = {0};

        //格式化日志名中的时间部分
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        //如果时间不是今天，则创建今天的日志，更新m_today和m_count;
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", m_dir_name, tail, m_log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        //如果超过最大行 在之前的日志名基础上加后缀, m_count/m_split_lines
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", m_dir_name, tail, m_log_name, m_count / m_max_lines);
        }
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    //写入内容格式： 时间+内容
    //时间格式化
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    //内容格式化
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();

    //若异步 则将日志信息加入阻塞队列 同步加锁向文件也如
    if(m_is_async && !m_log_queue->full())
        m_log_queue->push(log_str);
    else
    {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}