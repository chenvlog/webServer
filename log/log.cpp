#include  <string.h>
#include  <time.h>
#include  <sys/time.h>
#include  <stdarg.h>
#include  "log.h"
#include  <pthread.h>
using namespace std;

//构造函数
Log::Log(){
    m_count = 0;                //日志行数记录
    m_is_async = false;         //是否同步标志位
}

Log::~Log(){
    if(m_fp != NULL){
        fclose(m_fp);           //关闭文件描述符
    }
}

//异步需要设置阻塞队列的长度,同步不需要设置
bool Log::init(const char *file_name,int close_log,
        int log_buf_size=8192,int split_lines=5000000,int max_queue_size=0){
        
        //如果设置了max_queue_size，则为异步
        if(max_queue_size >= 1){
            m_is_async = true;
            m_log_queue = new block_queue<string>(max_queue_size);//设置阻塞队列的长度
            //创建线程专门去处理业务
            pthread_t tid;
            pthread_create(&tid,NULL,flush_log_thread,NULL);
        }

       
        m_close_log = close_log;            //关闭文件
        m_log_buf_size = log_buf_size;      //日志缓冲区大小
        m_buf = new char[m_log_buf_size];   //申请内存
        memset(m_buf,'\0',m_log_buf_size);  //初始化
        m_split_lines = split_lines;    //日志最大行数


        //处理文件名42-59
        time_t t = time(NULL);              //以当前时间为种子,产生随意数
        struct tm *sys_tm = localtime(&t);  //使用 t 的值来填充 tm 结构
        struct tm  my_tm  = *sys_tm;
        //file_name 所指向的字符串中搜索最后一次出现字符 '/'
        //扣出文件名和路径
        const char *p = strrchr(file_name,'/');
        char log_full_name[256] = {0};

        if(p == NULL){//只有文件名
            snprintf(log_full_name,255,"%d_%02d_%02d_%s",my_tm.tm_year+1900,my_tm.tm_mday,file_name);
        }else{

            strcpy(log_name,p+1);  //提取出日志名
            strncpy(dir_name,file_name,p - file_name + 1);//p - file_name + 1拷贝的长度
            snprintf(log_full_name,255,"%s%d_%02d_%02d_%s",dir_name,my_tm.tm_year+1900,my_tm.tm_mday,log_name);

        }

        m_today = my_tm.tm_mday;        //哪一天
        //"a"追加到一个文件
        m_fp = fopen(log_full_name,"a");
        if(m_fp == NULL){
            return false;
        }


        return true;

}
void Log::write_log(int level,const char *format,...){

    //获取当前时间
    struct timeval now = {0,0};
    gettimeofday(&now,NULL);//返回两个时间相差的秒数  获取精确的时间,或者执行计时
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t); //使用 t 的值来填充 tm 结构
    struct tm  my_tm  = *sys_tm;
    char  s[16] = {0};

    switch(level){//文件信息的类型
        case 0:
            strcpy(s,"[debug]:");
            break;
        case 1:
            strcpy(s,"[info]:");
            break;
        case 2:
            strcpy(s,"[warn]:");
            break;
        case 3:
            strcpy(s,"[error]:");
            break;
        default:
            strcpy(s,"[info]:");
            break;
    }

    //写入一个log  对m_count++,m_split_lines最大行数
    m_mutex.lock();
    m_count++;                  //日志行数记录
    if(m_today != my_tm.tm_mday || m_count % m_split_lines == 0){
            char new_log[256] = {0};
            fflush(m_fp);               //刷新缓冲区
            fclose(m_fp);               //关闭文件描述符
            char tail[16] = {0};        //xxxx_xx_xx

            snprintf(tail,16,"%d_%02d_%02d_",my_tm.tm_year+1900,my_tm.tm_mon,my_tm.tm_yday);
            if(m_today != my_tm.tm_mday){//如果不是今天
                snprintf(new_log,255,"%s%s%s",dir_name,tail,log_name);
                m_today = my_tm.tm_mday;
                m_count = 0;
            }else{
                //m_count/m_split_lines
                snprintf(new_log,255,"%s%s%s.%lld",dir_name,tail,log_name,m_count/m_split_lines);
            }
            m_fp = fopen(new_log,"a");
    }

    m_mutex.unlock();
    va_list valst;
    va_start(valst,format);

    string log_str;
    m_mutex.lock();

    //写入具体的时间内容格式
    int n = snprintf(m_buf,48,"%d-%02d-%-02d %02d:%02d:%02d:%02d.%06ld %s",
                     my_tm.tm_year + 1900,my_tm.tm_mon+1,my_tm.tm_mday,
                    my_tm.tm_hour,my_tm.tm_min,my_tm.tm_sec,now.tv_usec,s);

    int m = vsnprintf(m_buf+n,m_log_buf_size-1,format,valst);
    m_buf[n+m] = '\n';
    m_buf[n+m+1] = '0';
    log_str = m_buf;

    m_mutex.unlock();

    if(m_is_async && !m_log_queue->full()){//同步并且阻塞队列没有满  
        m_log_queue->push(log_str);
    }else{
        m_mutex.lock();
        fputs(log_str.c_str(),m_fp);
        m_mutex.unlock();
    }
    va_end(valst);
}
void Log::flush(void){
    m_mutex.lock();
    //强制刷新写入缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}