#pragma once
#include<fstream>
#include<string>
#include<string.h>
#include<time.h>
#include<unistd.h>
#include<sys/stat.h>
#include<mutex>
#include <unistd.h>
#include <sys/syscall.h>



class Logfile
{
protected:
    std::ofstream fout_;
    char filename_[50];
    std::ios::openmode mode_;
    time_t time_ = time(0);
    struct tm* tm_;
    char str_time_[20];//yyyy-mm-dd hh:MM:ss
    std::mutex write_mtx_;
    static std::mutex mtx_;
    static Logfile* object_ptr_;

protected:
    //默认构造函数
    Logfile() = default;
    

public:
    ~Logfile();
    //单例模式
    static Logfile* GetObject();

    //打开日志文件,参数依次为文件名、打开方式
    void Open(const char* filename,std::ios::openmode mode = std::ios::app);
    std::string GetFilename();
    
    //格式化写，用法同printf
    //格式：2024-03-15 12:00:09 内容
    template<typename... Args>
    void Write(const char* cstr,Args... args)
    {
        char buffer[200];
        char out[300];
        memset(buffer,0,sizeof(buffer));
        memset(out,0,sizeof(out));

        sprintf(buffer,cstr,args...);
        UpdateTime();
        out[0] = '[';                   //更新时间
        strcat(out,str_time_);        //时间
        strcat(out,"] [ThreadID: ");                //空格
        char threadid[8] = {0};
        sprintf(threadid,"%ld] ",syscall(SYS_gettid));
        strcat(out,threadid);
        strcat(out,buffer);
        strcat(out,"\n");              //要写入日志的内容
        
        {
            std::lock_guard<std::mutex> lg(write_mtx_);
            fout_.write(out,strlen(out));  //写入日志
        }
        
    }

    //获取本地时间
    void UpdateTime();

    bool MakeDir(const std::string& path,bool bisfilename);

    Logfile& operator<<(const std::string& str);


};
