#ifndef LOG_H
#define LOG_H

#include <stdio.h>         //编译器定义的头文件-->环境变量
#include <iostream>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;


//构造函数并不是线程安全的
//整个系统都在使用同一Log实例,使用单例模式
class Log  //使用单例模式,可以有效减少对象创建的时间
{
public:
    //c++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance(){
        static Log insertce;   //insertce插入
        return &insertce;
    }

    static void *flush_log_thread(void *args){//线程处理函数
        Log::get_instance()->async_write_log();
    }

    //可选择的参数有日志文件、日志缓冲区大小、最大行数、最长日志条队列
    bool init(const char *file_name,int close_log,int log_buf_size=8192,int split_lines=5000000,int max_queue_size=0);
    void write_log(int level,const char *format,...);
    void flush(void);

private:
    Log(/* args */);
    //父类的析构函数定义为虚函数(多态),否则子类的析构函数不会被触发,导致内存泄漏
    virtual ~Log();         
    void *async_write_log(){
        string single_log;
        while(m_log_queue->pop(single_log)){
            m_mutex.lock();
            fputs(single_log.c_str(),m_fp);
            m_mutex.unlock();
        }
    }
private:
    char                dir_name[128];          //路径名
    char                log_name[128];          //log文件名
    int                 m_split_lines;          //日志最大行数
    int                 m_log_buf_size;         //日志缓冲区大小
    long  long          m_count;                //日志行数记录
    int                 m_today;                //因为按天分类,记录当前时间的哪一天

    FILE                *m_fp;                  //打开log文件的指针
    char                *m_buf;                 //缓冲-->方便读文件和写文件
    block_queue<string> *m_log_queue;           //阻塞队列
    bool                 m_is_async;            //是否同步标志位  async异步
    locker               m_mutex;
    int                  m_close_log;           //关闭日志

};


//关闭文件？？？？ 信息的类型
#define LOG_DEBUG(format, ...) if(0 ==  m_close_log) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0  ==  m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0  ==  m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 ==  m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}



#endif