#include<iostream>
#include<pthread.h>
#include<stdio.h>
using namespace std;

//线程非安全版本只适合单线程
class single
{
private:
    single(){};
    ~single(){};
    static single *m_instance;

public:
    static single *getinstance();
};

single *single::m_instance = NULL;
single *single::getinstance()
{
    if(m_instance==NULL)
    {
        m_instance = new single;
    }
    return m_instance;
}

/*-------------------------------------------------------------------*/

//线程安全版本 单检测和双检测
class single2
{
private:
    single2()
    {
        pthread_mutex_init(&lock, NULL);
    };
    ~single2(){};

    static single2 *m_instance;
    static pthread_mutex_t lock;

public:
    static single2 *getinstance();
};

single2 *single2::m_instance = NULL;
pthread_mutex_t single2::lock;

single2 *single2::getinstance()
{
    //单检测
    pthread_mutex_lock(&lock);
    if(m_instance==NULL)
    {
        m_instance = new single2();
    }
    pthread_mutex_unlock(&lock);
    return m_instance;

    //双检测
    if(m_instance==NULL)
    {
        pthread_mutex_lock(&lock);
        if(m_instance==NULL)
            m_instance = new single2();
        pthread_mutex_unlock(&lock);
    }
    return m_instance;
}

/*-------------------------------------------------------------------*/
//线程安全版本局部静态变量
class single3
{
private:
    single3(){};
    ~single3(){};
public:
    static single3 *getinstance();
};

single3*single3::getinstance()
{
    //通过反汇编 有一个guard标志变量，如果第一个字节为0，代表instance为初始化
    //否则已经初始化
    static single3 instance;
    return &instance;
}