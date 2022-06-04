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
        
    }
}