#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<list>
#include<unistd.h>
using namespace std;

template <typename T>
class threadpool
{
public:
    threadpool(int thread_num, int max_requests);
    ~threadpool();

    bool append(T *requeest);

private:
    static void *worker(void *arg);
    void run();

private:
    int m_pthread_num;//线程池中的线程数量
    int m_max_requests;
    list<T *> m_workqueue; //请求队列
    pthread_t *m_thread;
    pthread_mutex_t m_queuelock; //保护请求队列的锁
    sem_t m_tasknum; //信号量代表任务数量
};

template<typename T>
threadpool<T>::threadpool(int thread_num,int max_requests): m_pthread_num(thread_num),m_max_requests(max_requests)
{
    pthread_mutex_init(&m_queuelock, NULL);
    sem_init(&m_tasknum, 0, 0);

    if (thread_num <= 0 || max_requests <= 0)
    {
        throw runtime_error("数量请求必须大于0");
    }
    m_thread = new pthread_t[thread_num];
    if(!m_thread)
        throw runtime_error("new线程池失败");

    for (int i = 0; i < thread_num;++i)
    {
        if(pthread_create(m_thread+i,NULL,worker,this)!=0)
        {
            delete[] m_thread;
            throw runtime_error("线程创建失败");
        }
        cout << "创建线程：" << i <<endl;
        if (pthread_detach(m_thread[i]) != 0)
        {
            delete[] m_thread;
            throw runtime_error("线程分离失败");
        }
    }

    /*-----------用于测试------------*/
    cout << "线程池构造函数" << endl;
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete[] m_thread;
    pthread_mutex_destroy(&m_queuelock);
    sem_destroy(&m_tasknum);

    /*-----------用于测试------------*/
    cout << "线程池析构函数" << endl;
}

template<typename T>
bool threadpool<T>::append(T *requeset)
{
    pthread_mutex_lock(&m_queuelock);

    if(m_workqueue.size()>=m_max_requests)
    {
        pthread_mutex_lock(&m_queuelock);
        cout << "工作线程队列已满" << endl;
        return false;
    }
    m_workqueue.push_back(requeset);
    pthread_mutex_unlock(&m_queuelock);
    sem_post(&m_tasknum); //任务数量+1
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run()
{
    while(true)
    {
        sem_wait(&m_tasknum);  //阻塞等待任务
        pthread_mutex_lock(&m_queuelock);

        if(m_workqueue.empty())
        {
            pthread_mutex_unlock(&m_queuelock);
            continue;
        }

        T *request = m_workqueue.front();//取出一个任务
        m_workqueue.pop_front();
        pthread_mutex_unlock(&m_queuelock);

        if(!request)
            continue;

        cout << "线程:"<<pthread_self()<<" 开始处理任务" <<*request<<" pid:"<<getpid()<< endl;
        /*具体的任务逻辑*/
    }
}
#endif