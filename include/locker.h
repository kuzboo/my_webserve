#ifndef LOCKER_H
#define LOCKER_H

#include<exception>
#include<pthread.h>
#include<semaphore.h>
#include<time.h>
using namespace std;

//封装信号量的类
class sem
{
public:
    sem()//初始化一个信号量
    {
        if(sem_init(&m_sem,0,0)!=0)
            throw exception();
    }
    sem(int num)//初始化一个初值为num的信号量
    {
        if(sem_init(&m_sem,0,num)!=0)
            throw exception();
    }
    ~sem()//销毁信号量
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;//信号量-1 信号量为0时 sem_wait将阻塞
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;//信号量+1 信号量大于0时唤醒调用sem_post的线程
    }

private:
    sem_t m_sem;
};

//封装互斥锁的类
class locker
{
public:
    locker()
    {
        if(pthread_mutex_init(&m_mutex,0)!=0)
        {
            throw exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    //获取互斥锁的地址
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

//封装条件变量的类
class cond
{
public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            throw exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    //调用前需要将mutex加锁
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        /*
        1.阻塞等待条件变量cond满足
        2.释放已经掌握的互斥锁（相当于unlock）这两步为原子操作
        3.当条件满足函数返回时重新申请互斥锁（相当于lock）
        */
        ret = pthread_cond_wait(&m_cond, m_mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex,struct timespec t)
    {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        return ret == 0;
    }
    //唤醒单个线程
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    //通过广播唤醒多个线程
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;
};
#endif