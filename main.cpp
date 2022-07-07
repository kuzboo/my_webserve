#include<string>

#include"config.h"

using namespace std;

int main(int argc, char *argv[])
{
    string user = "root";
    string passwd = "root";
    string databasename = "yhbdb";

    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;
    server.init(config.PORT, user, passwd, databasename, config.LOGWrite, config.OPT_LINGER, config.TRIMode,
                config.SQLNUM, config.THREADNUM, config.CLOSELOG, config.ACTORMODE);
    server.log_write();
    server.sql_pool();
    server.thread_pool();
    server.trig_mode();
    server.eventlisten();
    server.eventLoop();

    return 0;
}