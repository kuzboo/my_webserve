#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<list>
using namespace std;

template<typename T>
class threadpool
{
public:
    threadpool(int thread_num, int max_request);
    ~threadpool();

    bool append(T *request);

private:
    static void *worker(void *arg);
    void run();

private:
    int m_thread_num;
    int m_max_request;
    list<T *> m_workqueue;
    pthread_t *m_thread;
    pthread_mutex_t m_queuelock;
    sem_t m_task_num;
};

template <typename T>
threadpool<T>::threadpool(int thread_num, int max_request)
    : m_thread_num(thread_num), m_max_request(max_request)
    {
        pthread_mutex_init(&m_queuelock, NULL);
        sem_init(&m_task_num, 0, 0);

        if(thread_num<=0 || max_request<=0)
            throw runtime_error(" 线程数量 请求数量");
        m_thread = new pthread_t(thread_num);
        if(!m_thread)
            throw exception();

        for (int i = 0; i < m_thread_num;++i)
        {
            if(pthread_create(m_thread+i,NULL,worker,this)!=0)
            {
                delete[] m_thread;
                throw exception();
            }
            if(pthread_detach(m_thread[i])!=0)
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
    pthread_mutex_destroy(&m_queuelock);
    sem_destroy(&m_task_num);
}

template<typename T>
bool threadpool<T>::append(T* request)
{
    pthread_mutex_lock(&m_queuelock);

    if(m_workqueue.size()>=m_max_request)
    {
        pthread_mutex_unlock(&m_queuelock);
        cout << "工作线程已满" << endl;
        reutrn false;
    }
    m_workqueue.push_back(request);
    pthread_mutex_unlock(&m_queuelock);
    sem_post(&m_task_num);
    return true;
}

template<typename T>
void* threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while (true)
    {
        sem_wait(&m_task_num);
        pthread_mutex_lock(&m_queuelock);

        if(m_workqueue.empty())
        {
            pthread_mutex_unlock(&m_queuelock);
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        pthread_mutex_unlock(&m_queuelock);

        if(!request)
            continue;

        cout << "开始处理任务" << endl;
        /*具体的任务逻辑*/
    }
}

#endif