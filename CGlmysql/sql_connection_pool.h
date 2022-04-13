#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

/*
    Mysql数据库交互
*/

#include <mysql/mysql.h>
#include <iostream>
#include <list>
#include <string>
#include "lock/locker.h"
#include "log/log.h"


using namespace std;

//单例模式-->一个类只有一个对象
class connection_pool
{
public:
    //获取数据库连接
    MYSQL   *GetConnection();                   //获取数据库连接
    bool     ReleaseConnection(MYSQL *conn);    //释放连接
    int      GetFreeConn();                     //获取当前空闲连接数目,对外接口
    void     DestroyPool();                     //销毁所有连接

   
    static connection_pool *GetInstance();//定义一个连接池,并返回这个链接池的指针
    void   init(string url,string User,string PassWord,string DataBaseName,int Port,int MaxConn,int close_log);

private:
    //构造函数私有化
    connection_pool();
    ~connection_pool();

    int m_MaxConn;              //最大连接数
    int m_CurConn;              //当前已使用的连接数
    int m_FreeConn;             //当前空闲的连接数
    locker lock;
    list<MYSQL *>connList;      //装连接池中的连接
    sem reserve;                //空闲的连接数
public:
    string m_url;               //主机地址
    string m_Port;              //数据库端口号
    string m_User;              //登录数据库用户名
    string m_PassWord;          //登录数据库密码
    string m_DatabaseName;      //使用数据库名
    int    m_close_log;         //日志开关
};

//用来对外管理链接池中的单个链接管理-->获取链接,关闭连接
class connectionRAII{
public:
    connectionRAII(MYSQL **SQL,connection_pool *connpool);
    ~connectionRAII();
private:
    MYSQL *connRAII;
    connection_pool *poolRAII;
};


#endif