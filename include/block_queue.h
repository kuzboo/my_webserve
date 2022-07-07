#ifndef BLOCK_QUEUE
#define BLOCK_QUEUE

#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include"locker.h"
using namespace std;

template<class T>
class block_queue
{
public:
    block_queue(int max_size = 1000)
    {
        if(max_size<=0)
            exit(-1);
        
        //构造循环数组
        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    ~block_queue()
    {
        m_mutex.lock();
        if(m_array!=NULL)
            delete[] m_array;
        m_mutex.unlock();
    }

    void clear()
    {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    bool full()
    {
        m_mutex.lock();
        if (m_size >= m_max_size)
        {

            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty()
    {
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool back(T &value)
    {
        m_mutex.lock();
        if(!m_size)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[back];
        m_mutex.unlock();
        return true;
    }

    int size()
    {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }
    int max_size()
    {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_max_size;

        m_mutex.unlock();
        return tmp;
    }
    /*
    【生产者】往队列添加元素，需要将所有使用队列的线程先唤醒
    当有元素push进队列，相当于生产者生产了一个元素
    若当前没有线程等待条件变量，则唤醒无意义
    */
    bool push(const T &item)
    {
        m_mutex.lock();

        if (m_size >= m_max_size)
        {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
       //将新增数据放在循环数组对应位置
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        m_size++;

        m_cond.broadcast();
        m_mutex.unlock();

        return true;
    }
    //【消费者】pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item)
    {
        m_mutex.lock();
        while(m_size<=0) //条件不满足
        {
            if(!m_cond.wait(m_mutex.get()))//阻塞等待
            {
                m_mutex.unlock();
                return false;
            }
        }
        //条件满足 取出队列首元素
        m_front = (m_front + 1) % m_max_size;//【？？？？】
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

private: 
    locker m_mutex;
    cond m_cond;

    T *m_array;
    int m_size;
    int m_max_size;
    int m_front; //队首索引
    int m_back;  //队尾索引
};

#endif