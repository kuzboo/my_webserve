#include"threadpool.h"
using namespace std;

int main()
{
    cout << "main ID：" << pthread_self() << ":" << getpid() << endl;
    threadpool<int> *pool = new threadpool<int>(10, 10);
    for (int i = 0; i < 10;++i)
    {
        pool->append(&i);
        sleep(1);
    }
    sleep(2);
    delete pool;
    return 0;
}
