#include<iostream>

#include"../include/connection_pool.h"
using namespace std;

connection_pool::connection_pool() : m_curconn(0), m_freeconn(0){};

connection_pool::~connection_pool() { DestroyPool(); };

connection_pool* connection_pool::GetInstance()
{
    static connection_pool obj;
    return &obj;
}

//连接池初始化
void connection_pool::init(string Url, string User, string PassWord,
                           string DataBserName, int Port, int MaxConn, int close_log)
{
    m_Url = Url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DataBaseName = DataBserName;
	m_close_log = close_log;

    //创建maxconn条数据库连接
    for (int i = 0; i < MaxConn;++i)
    {
        MYSQL *con = NULL;
        con = mysql_init(con);

        if(con==NULL)
        {
            cout << "Error:" << mysql_error(con);
            exit(1);
        }
        con = mysql_real_connect(con, Url.c_str(), User.c_str(), PassWord.c_str(),
                                 DataBserName.c_str(), Port, NULL, 0);
        if(con==NULL)
        {
            exit(1);
        }
        
        //更新连接池和空闲连接数量
        connlist.push_back(con);
        ++m_freeconn;
    }

    //将信号量初始化为最大连接次数
    reserve = sem(m_freeconn);

    m_maxconn = m_freeconn;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* connection_pool::GetConnection()
{
    MYSQL *con = NULL;

    if(connlist.empty())
        return NULL;
    
    //取出连接 信号量原子减一 为0则阻塞
    reserve.wait();
    m_mutex.lock();

    con = connlist.front();
    connlist.pop_front();

    --m_freeconn;
    ++m_curconn;

    m_mutex.unlock();
    return con;
}

//释放当前使用的连接，push_back到连接池
bool connection_pool::ReleaseConnection(MYSQL *conn)
{
    if(conn==NULL)
        return false;

    m_mutex.lock();

    connlist.push_back(conn);
    ++m_freeconn;
    --m_curconn;

    m_mutex.unlock();
    reserve.post();

    return true;
}

//销毁连接池
void connection_pool::DestroyPool()
{
    m_mutex.lock();

    if(connlist.size()>0)
    {
        list<MYSQL *>::iterator it;
        for (it = connlist.begin(); it != connlist.end(); ++it)
        {
            MYSQL *conn = *it;
            mysql_close(conn);
        }
        m_curconn = 0;
        m_freeconn = 0;
    }
    connlist.clear();

    m_mutex.unlock();
}

int connection_pool::GetFreeConn()
{
    return m_freeconn;
}

//RAII机制获取/释放数据库连接
connectionRAII::connectionRAII(MYSQL **SQL,connection_pool *connPool)
{
    *SQL = connPool->GetConnection();
    connRAII = *SQL;
    poolRAII = connPool;
}
connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(connRAII);
}