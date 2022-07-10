#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>

//链表作为共享数据，需要互斥量保护
typedef struct msg{
    struct msg *next;
    int num;
} msg;
msg *head;

//静态初始化一个互斥量和条件变量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//消费者
void *consumer(void *p)
{
    msg *mp;
    for (;;)
    {
        //因为要访问共享区 所以要加锁
        pthread_mutex_lock(&mutex);

        while(head==NULL)
        {
            pthread_cond_wait(&cond, &mutex);
        }

        //有资源了开始消费,消费一个节点
        mp = head;
        head = mp->next;

        pthread_mutex_unlock(&mutex);

        //打印输出信息(pthread_self()返回线程自身id)
        printf("-Consume %lu---%d,pid:%d\n", pthread_self(), mp->num,getpid());

        free(mp);
        sleep(rand() % 5);
    }
}

//生产者
void *producer(void *arg)
{
    msg *mp;
    for (;;)
    {
        mp = malloc(sizeof(msg));
        mp->num = rand() % 1000 + 1;
        printf("-Producer %lu--%d,pid:%d\n", pthread_self(), mp->num,getpid());

        pthread_mutex_lock(&mutex);
        mp->next = head;
        head = mp;
        pthread_mutex_unlock(&mutex);

        //唤醒一个消费者
        pthread_cond_signal(&cond);
        sleep(rand() % 5);
    }
}

int main()
{
    pthread_t pid, cid;
    srand(time(NULL));

    pthread_create(&pid, NULL, producer, NULL);//生产者
    pthread_create(&cid, NULL, consumer, NULL);//消费者

    pthread_join(pid, NULL);
    pthread_join(cid, NULL);
    return 0;
}