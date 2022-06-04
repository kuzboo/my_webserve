#ifndef LOG_H
#define LOG_H

#include<stdio.h>
#include<pthread.h>
#include<string>
#include"../include/block_queue.h"

using namespace std;

class Log
{
public:
    //c++11后，使用局部变量懒汉模式不用枷锁
    static Log* get_instance()
    {
        static Log obj;
        return &obj;
    }

    //日志文件，缓冲区大小，最大行数，最长日志条队列
    bool init(const char *file_name, int log_buf_size = 8192, int max_lines = 5000000, int max_queue_size = 0);

    //异步写入日志公有方法 内部调用私用方法async_write_log(回调函数)
    static void *flush_log_thread(void *args)
    {
        //类名+：：调用静态方法
        Log::get_instance()->async_write_log();
    }

    //输出内容按照标准给是整理
    void write_log(int level, const char *format, ...);
    //强制刷新缓冲区
    void flush(void);

private:
    Log()
    {
        m_count = 0;
        m_is_async = false;
    }
    virtual ~Log()
    {
        if(m_fp!=NULL)
            fclose(m_fp);
    }

    //异步写日志 从阻塞队列获取一个日志string写入文件
    void *async_write_log()
    {
        string single_log;
        while(m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128]; //路径名
    char log_name[128]; // log文件名;
    int m_max_lines;    //日志最大行数;
    int m_log_buf_size; //日志缓冲区大小;
    long long m_count;  //日志行数记录;
    int m_date;        //记录当前时间是那一天;
    FILE *m_fp;        //打开log的文件指针<stdio.h>;
    char *m_buf;       //要输出的内容;
    block_queue<string> *m_log_queue; //阻塞队列;
    bool m_is_async;                  //是否同步标志位;
    locker m_mutex;
};

#endif