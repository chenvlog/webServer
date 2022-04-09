#ifndef LOCKER_K
#define LOCKER_H

#include <exception>        //异常捕捉
#include <pthread.h>        //线程
#include <semaphore.h>

//将信号量 互斥锁 条件变量封装起来
class sem{

public:
    //初始化:创建一个m_sem
    sem(){
        if(sem_init(&m_sem,0,0) != 0){//初始化一个信号量
            throw std::exception();
        }
    }
    ~sem(){//释放信号量资源
        sem_destroy(&m_sem);
    }
    //减1  wait
    bool wait(){
        return sem_wait(&m_sem)==0;
    }

    //加1  post
    bool post(){
        return sem_post(&m_sem)==0;
    }

private:
    sem_t m_sem;

};


//互斥量   上锁和解锁
class locker
{
public:
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL) != 0){
             throw std::exception();
        }
    }
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    //上锁
    bool lock(){
        return pthread_mutex_lock(&m_mutex)==0;
    }

    //解锁
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }

    //做一个接口,让外面可以得到这把锁
    pthread_mutex_t *get_mutex(){
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

//条件变量
/*
    条件变量不是锁,但是可以使线程阻塞。
    当满足一定条件的时候才唤醒阻塞的线程,换句话说就是一个线程阻塞,等待另一个线程去救赎它
    在消费者和生产者模式中,当缓冲区没有资源的时候,消费者与消费者之间的竞争是毫无意义的
    所以可以使用条件变量有效的去控制
*/
class cond{
public:   
    cond(){
        if(pthread_cond_init(&m_cond,NULL) != 0){
            throw std::exception();
        }
    }
    ~cond(){
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *m_mutex){//阻塞等待
        return pthread_cond_wait(&m_cond,m_mutex)==0;
    }

    bool wait_time(pthread_mutex_t *m_mutex,struct timespec t){//定时等待
        return pthread_cond_timedwait(&m_cond,m_mutex,&t) == 0;
    }

    bool signal(){//当缓冲区有东西了,就可以让消费者去争取资源
        return pthread_cond_signal(&m_cond);
    }

private:
    pthread_cond_t m_cond;
};



#endif