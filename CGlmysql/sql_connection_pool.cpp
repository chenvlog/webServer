#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"
//构造函数私有化
connection_pool::connection_pool(){
    m_CurConn = 0;                  //当前已使用的连接数
    m_FreeConn = 0;                 //当前空闲的连接数
}

connection_pool *connection_pool::GetInstance(){
    static connection_pool connPool;
    return &connPool;
}

connection_pool::~connection_pool(){
    DestroyPool();
}
//初始化
/*
    创建MaxConn个连接并加入到连接队列中
    设置sem = 空闲的连接数   ==>  线程同步带来的问题
*/
void  connection_pool::init(string url,string User,string PassWord,string DataBaseName,int Port,int MaxConn,int close_log){
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DataBaseName;
    m_close_log = close_log;

    for(int i=0;i<MaxConn;i++){//创建MaxConn个连接并加入到连接队列中
        MYSQL *con = NULL;
        con = mysql_init(con);

        if(con == NULL){
            LOG_ERROR("MySQL Error");
        }

        con = mysql_real_connect(con,url.c_str(),User.c_str(),PassWord.c_str(),DataBaseName.c_str(),Port,NULL,0);
        if(con == NULL){
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        connList.push_back(con);
        ++m_FreeConn;
    }

    reserve   =  sem(m_FreeConn);
    m_MaxConn =  m_FreeConn;
}

//当有请求时,从数据库连接池中返回一个可用连接,更新使用和空闲连接数
MYSQL *connection_pool::GetConnection(){
    MYSQL *con = NULL;
    if(connList.size() == 0){
        return NULL;
    }

    reserve.wait(); //如果线程sem==0则阻塞等待 sem-1

    lock.lock();
    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();
    return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con){
    if(con == NULL){
        return false;
    }
    lock.lock();
    connList.push_back(con);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();
    reserve.post(); //p sem+1
    return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool(){
    lock.lock();
    if(connList.size() > 0){
        list<MYSQL *>::iterator it;
        for(it = connList.begin();it != connList.end();++it){
            MYSQL *con = *it;
            mysql_close(con);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
    lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn(){
    return this->m_FreeConn;
}

connectionRAII::connectionRAII(MYSQL **SQL,connection_pool *connPool){
    *SQL = connPool->GetConnection();//从这个数据库获取一个链接
    connRAII = *SQL;
    poolRAII = connPool;
}
connectionRAII::~connectionRAII(){
    poolRAII->ReleaseConnection(connRAII);
}