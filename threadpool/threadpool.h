#ifndef THREADPOOL_H
#define THREADPOOL_h

/*
    线程池
*/
#include <pthread.h>
#include "CGlmysql/sql_connection_pool.h"
template<typename T>
class threadpool{

public:
    /*thread_number是线程池中线程的数量*/
    threadpool(int actor_model,connection_pool *connPool,int thread_number = 8,int max_request = 10000);
    ~threadpool();
    //增加请求事件
    bool append(T *request,int state); 
    bool append_p(T *request);

private:
    /*工作线程运行的函数,它不断从工作队列中取出任务并执行*/
    static void *worker(void *arg);
    void   run();

private:
    int              m_thread_number;        //线程池中的线程数
    int              m_max_requests;         //请求队列中允许的最大请求数
    pthread_t       *m_threads;              //保存线程的数组,最大数目是m_max_requests
    std::list<T *>   m_workqueue;            //请求队列
    locker           m_queuelocker;          //保护请求队列的互斥锁
    sem              m_queuestat;            //信号量,表示是否有任务需要处理
    connection_pool *m_connPool;             //数据库
    int              m_actor_model;          //模型切换        

};

template <typename T>
threadpool<T>::threadpool(int actor_model,connection_pool *connPool,int thread_number = 8,int max_request = 10000){

}


#endif

