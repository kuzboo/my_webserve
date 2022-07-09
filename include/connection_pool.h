#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include<list>
#include<string>
#include<mysql/mysql.h>
#include"locker.h"

class connection_pool
{
public:
    MYSQL *GetConnection();              //获取数据库连接;
    bool ReleaseConnection(MYSQL *conn); //释放数据库连接
    int GetFreeConn();                   //当前空闲连接数;
    void DestroyPool();                  //销毁所有连接;
    void init(string url, string User, string PassWord,
              string DataBserName, int Port, int MaxConn,int close_log); //初始化数据库连接;

    static connection_pool *GetInstance();//局部静态变量单例模式

    string m_Url;  //主机地址
    string m_Port; //数据库端口号
    string m_User; //登录数据库用户名
    string m_PassWord;     //登录数据库密码
    string m_DataBaseName; //使用数据库名
    int m_close_log;       //日志开关

private:
    connection_pool();
    ~connection_pool();

    int m_maxconn;  //最大连接数
    int m_curconn;  //当前连接数
    int m_freeconn; //当前空闲连接数
    locker m_mutex;
    list<MYSQL *> connlist; //连接池
    sem reserve;
};

class connectionRAII
{
public:
    connectionRAII(MYSQL **SQL, connection_pool *connPool);
    ~connectionRAII();
private:
    MYSQL *connRAII;           //一个数据库连接
    connection_pool *poolRAII; //数据库连接池对象
};

#endif