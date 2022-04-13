#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

/*  
    通过循环队列实现缓冲区 
    对缓冲区(共享资源)的操作-->线程成同步问题,所以要加锁、解锁

    我感觉只有读操作的画可以不用加互斥锁
*/


#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <stdlib.h>
#include "lock/locker.h"

using namespace std;

//T 表示要处理的事务,事务类型不确定,随意使用模板
template<class T>
class block_queue{

public:
    block_queue(int max_size = 1000){ //队列的最大容量
        if(max_size <= 0){//队列没有容量,线程永远无法工作
            exit(-1);
        }

        m_max_size  =   max_size;         //队列的最大容量
        m_array     =   new T[max_size];     //初始化一个数组,循环队列的准备
        m_size      =   0;
        m_front     =  -1;
        m_back      =  -1;
    }

    void  clear(){//清空队列
        
        /*同一时刻只需要一个线程去完成就行了*/
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    } 
    ~block_queue(){
        //这里一定要加锁,否则可能导致两次释放同一个资源
        m_mutex.lock();
        if(m_array != NULL){//这里一定要加判断,否则可能导致两次释放同一个资源
            delete [] m_array;  //把推上上的内存释放
        }
        m_mutex.unlock();
    }

    //接下来就是队列的常规功能了
    
    //判断队列有没有满
    bool full(){
        m_mutex_lock();
            if(m_size >= m_max_size){//满了
                m_mutex.unlock();
                return true;
            }
            m_mutex_unlock();
            return false;
    }
    //判断队列是否为空
    bool empty(){

        m_mutex.lock();
        if(m_size == 0){//确实为空
            m_mutex.unlock();
            return true;
        }

        m_mutex.unlock();
        return false;
    }

    //返回队列的首元素
    bool front(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    //返回队尾元素
    bool back(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;        
    }

    //获取队列的大小
    int size(){
        int tmp = 0;
        m_mutex.lock();
            tmp = m_size;
            m_mutex.unlock();
        m_mutex.unlock();
        return tmp;
    }

    //获取队列目前最大的大小
    int max_size(){
        int tmp = 0;
        m_mutex.lock();
            tmp = m_max_size;
            m_mutex.unlock();
        m_mutex.unlock();
        return tmp;
    }

    //向缓冲区放一个物品,当缓冲区有物品的时候
    //所有的正在等待的线程都要去抢夺,防止某些线程会被饿死
    bool push(const T &item){

        m_mutex.lock();
        if(m_size >= m_max_size){//缓冲区放不下了
            m_cond.broadcast();  //广播通知所有的正在等待的线程
            m_mutex.unlock();
            return false;
        }

        m_back =  (m_back + 1)%m_max_size;
        m_array[m_back] = value;

        m_cond.broadcast();  //广播通知所有的正在等待的线程
        m_mutex.unlock();
        return false;

    }

    //pop,如果当前队列中没有元素,将会等待条件变量
    bool pop(T &item){
        m_mutex.lock();
        while(m_size <= 0){//队列里没有元素
            if(!m_cond.wait(m_mutex.get())){//线程阻塞在门口
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }
    //pop 超时处理
    bool pop(T &item,int ms_timeout){
        struct timespec t = {0,0};   
        struct timeval  now = {0,0};  

        //int gettimeofday(struct timeval*tv, struct timezone *tz);
        //gettimeofday  获取精确的时间,或者执行计时
        gettimeofday(&now,NULL);
        m_mutex.lock();
        if(m_size <= 0){
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if(!m_cond.wait_time(m_mutex.get(),t)){//定时等待t
                m_mutex.unlock();
                return false;
            }
        }

        if(m_size <= 0){
            m_mutex.unlock();
            return false;
        }

        m_front = (m_front+1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

private:

    locker m_mutex;
    cond   m_cond;
    
    T *m_array;
    int m_max_size;
    int m_front;        //队头
    int m_back;         //队尾
    int m_size;         //队列的大小,当前有多少事务未处理

};












#endif BLOCK_QUEUE_H
