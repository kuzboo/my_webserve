#ifndef CONFIG_H
#define CONFIG_H

#include"../include/webserver.h"
using namespace std;

class config
{
private:
    /* data */
public:
    config(/* args */);
    ~config();

    void parse_arg(int argc, char *argv[]);
    int PORT;
    int LOGWrite;//日志写入方式
    int TRIMode;
    int LISTENTrigmode;//listen触发模式
    int CONNTrigmode;  // connect触发模式
    int OPT_LINGER;//优雅关闭链接
    int SQLNUM;
    int THREADNUM;
    int CLOSELOG;
    int ACTORMODE;
};

#endif