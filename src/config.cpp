#include"../include/config.h"

config::config()
{
    PORT = 9006;
    LOGWrite = 0;//默认同步
    TRIMode = 0; //触发模式默认listendfd LT+connfd LT
    LISTENTrigmode = 0;
    CONNTrigmode = 0;
    OPT_LINGER = 0;
    SQLNUM = 8;
    THREADNUM = 8;
    CLOSELOG = 0;
    ACTORMODE = 0;
}
void config::parse_arg(int argc, char *argv[])
{
    int opt;
    const char *str = "p:l:m:o:s:t:c:a";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            PORT = atoi(optarg);
            break;
        }
        case 'l':
        {
            LOGWrite = atoi(optarg);
            break;
        }
        case 'm':
        {
            TRIMode = atoi(optarg);
            break;
        }
        case 'o':
        {
            OPT_LINGER = atoi(optarg);
            break;
        }
        case 's':
        {
            SQLNUM = atoi(optarg);
            break;
        }
        case 't':
        {
            THREADNUM = atoi(optarg);
            break;
        }
        case 'c':
        {
            CLOSELOG = atoi(optarg);
            break;
        }
        case 'a':
        {
            ACTORMODE = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}