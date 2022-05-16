#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<iostream>
#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"locker.h"

template<typename T>
class threadpool
{
public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();

    bool append(T *request);
private:
    static void *worker(void *arg);//静态成员函数 
    void run();

private:
    int m_pthread_number;  //线程池中的线程数量
    int m_max_requests;    //请求队列的最大请求数量
    pthread_t *m_thread;   //线程池数组
    list<T *> m_workqueue; //请求队列
    locker m_queuelocker;  //保护请求队列的互斥锁;
    sem m_queuestat;       //是否有任务需要处理;
    bool m_stop;           //是否结束线程
};

//线程池的创建与回收
template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : m_pthread_number(thread_number), m_max_requests(max_requests)
{
    if(thread_number<=0 || max_requests<=0)
    {
        throw exception();
    }
    m_thread = new pthread_t[thread_number];
    if(!m_thread)
        throw exception();

    for (int i = 0; i < thread_number;++i)
    {
        //循环创建线程
        /*
        类对象传递时用this指针，传递给静态函数后，将其转换为线程池类，并调用私有成员函数run。
        构造函数中创建线程池,pthread_create函数中将类的对象作为参数传递给静态函数(worker),
        在静态函数中引用这个对象,并调用其动态方法(run)。
        */
        if (pthread_create(m_thread + i, NULL, worker, this) != 0) //为什么传this
        {
            delete[] m_thread;
            throw exception();
        }
        if(pthread_detach(m_thread[i])) //分离主线程子线程 资源自动回收
        {
            delete[] m_thread;
            throw exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete[] m_thread;
}

//向请求队列中添加任务
template<typename T>
bool threadpool<T>::append(T* request)
{
    m_queuelocker.lock();
    if(m_workqueue.size()>=m_max_requests)
    {
        m_queuelocker.unlock();
        cout << "请求队列已满" << endl;
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    //信号量提醒有任务要处理[信号量+1]
    m_queuestat.post();
    return true;
}

//线程处理函数 内部访问私有成员函数run
//如果该函数不设为静态函数，this指针会作为默认参数传进函数中与void*不匹配
template<typename T>
void* threadpool<T>::worker(void* arg)
{
    //强转成线程池类 然后内部调用工作线程
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

//run执行任务 主要实现工作现场从请求队列中取出某个任务进行处理
template<typename T>
void threadpool<T>::run()
{
    while(!m_stop)
    {
        m_queuestat.wait();//有任务需要处理 唤醒工作队列
        m_queuelocker.lock();//工作队列加锁
        if(m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }

        T *request = m_workqueue.front();//从工作队列中取出第一个任务
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request)
            continue;
    }
}

#endif