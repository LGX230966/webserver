#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include <sys/sem.h>
#include <functional>
#include <vector>
#include <sys/socket.h>
#include <netdb.h>
#include <regex>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/shm.h>


#define CREATE 0   //如果路径不存在，创造文件夹
#define READ 1     //如果路径不存在，不创造文件夹
using namespace std;
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
class DirOpe
{

protected:
    DIR *m_dir;
    struct dirent* m_st_entry;

    
public:
    //CREATE 0   如果路径不存在，创造文件夹
    //READ 1     如果路径不存在，不创造文件夹
    bool OpenDir(const string& dir_path,int model = CREATE);

    //b_is_filename 路径最后是否是文件
    bool MkDir(const string& path_or_filename,bool b_is_filename);

    //遍历并处理
    //绝对路径 处理函数(绝对路径，可变参数列表) 可变参数列表
    bool Traverse(const string& dir_path,bool include_subdir,void(*p)(const string&,va_list valist),...);

    //用于递归
    //绝对路径 处理函数(绝对路径，可变参数列表) 可变参数列表
    bool Traverse(const string& dir_path,bool include_subdir,void(*p)(const string&,va_list valist),va_list valist);
};

class FileIO
{
// protected:
public:
    fstream fio;

public:
    template<typename... Args>
    bool Write(const string content,Args... args)
    {
        char cbuffer[300];
        memset(cbuffer,0,sizeof(cbuffer));
        sprintf(cbuffer,content.c_str(),args...);
        fio.write(cbuffer,strlen(cbuffer));
        if(fio.bad())
        {
            fio.clear();
            return false;
        }
        return true;
    }

    //读取一行
    void ReadLine(string& buffer);

    void Read(string& buffer);

    void WriteLine(string& buffer);

    void Write(string& buffer);

    //用于创建目录
    static bool MakeDir(const string& path_or_filename,bool b_is_filename);

    //打开文件，若没有文件则创建文件
    bool Open(const char* file_path,std::ios::openmode mode = std::ios::in|std::ios::out|std::ios::app);

    //关闭文件流
    void Close();


    //有待完善
    bool XMLGet(const string attribute,string& buffer);

    bool _EOF();
};

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// class Logfile
// {
// protected:
//     ofstream m_fout;
//     char m_filename[50];
//     ios::openmode m_mode;
//     time_t m_time = time(0);
//     struct tm* m_st_tm;
//     char m_cstr_time[20];//yyyy-mm-dd hh:MM:ss

// public:
//     //默认构造函数
//     Logfile() = default;
//     ~Logfile();
    
//     //打开日志文件,参数依次为文件名、打开方式
//     void Open(const char* filename,ios::openmode mode = ios::app);
//     string GetFilename();
    
//     //格式化写，用法同printf
//     //格式：2024-03-15 12:00:09 内容
//     template<typename... Args>
//     void Write(const char* cstr,Args... args)
//     {
//         char buffer[200];
//         char out[219];
//         memset(buffer,0,sizeof(buffer));
//         memset(out,0,sizeof(out));

//         sprintf(buffer,cstr,args...);
//         UpdateTime();                   //更新时间
//         strcat(out,m_cstr_time);        //时间
//         strcat(out," ");                //空格
//         strcat(out,buffer);             //要写入日志的内容
        
//         m_fout.write(out,strlen(out));  //写入日志
//     }

//     //获取本地时间
//     void UpdateTime();

//     bool MakeDir(const string& path,bool bisfilename);

//     Logfile& operator<<(const string& str);


// };

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
union semun {
    int val;                // 用于 SETVAL 命令，设置信号量的值
    struct semid_ds *buf;    // 用于 IPC_STAT 和 IPC_SET 命令，操作信号量集的属性
    unsigned short *array;   // 用于 GETALL 和 SETALL 命令，获取或设置信号量集中所有信号量的值
    struct seminfo *__buf;   // 用于 IPC_INFO 命令，获取内核信号量信息
    
};

class Sem
{
protected:
    key_t m_key;                //创建或获取信号量的键
    int m_semid;                  //信号量的id
    struct sembuf m_st_sem_opa; //存放semop所需的参数
    semun un_semun;             //存放semctl所需的参数
public:
    // Sem(key_t key = 0x1234 ,int val = 1);
    Sem();
    ~Sem();

    //初始化函数，用于获取信号量id
    //key:信号量的键 val:信号量的值
    bool Init(key_t key = 0x1234,int val = 1);
    
    //申请信号量
    bool Wait();

    //释放信号量
    bool Signal();

    //设置信号量的值
    bool SetVal(int val);

    //返回信号量的值
    int GetVal();

    //删除信号量
    bool Close();
};

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


struct procinfo
{
    int pid;
    char pname[51];
    int timeout=0;
    time_t atime=0;
    procinfo() = default;
    procinfo(const int in_pid,const string & in_pname,const int in_timeout,const time_t in_atime)
                :pid(in_pid),timeout(in_timeout),atime(in_atime)
    {
        strncpy(pname,in_pname.c_str(),50);//\0占一个索引
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

class Pactive
{
protected:
    int m_shmid;
    struct procinfo* m_shm;
    int m_pos; //结构体在共享内存中的偏移量
    Sem sem;

public:
    
    //信号处理函数
    //进程名，活动周期，时限，共享内存key，信号量key
    void Init(string p_name,int act_cycle = 5,time_t timeout = 30,key_t key = 0x5095,key_t sem_key = 0x1234);
    Pactive();
    ~Pactive();
    void EXIT(int sig = 0);
};

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//分割函数，用于c++风格字符串
//str 需要分割的字符串
//v_str 存放分割后的子串的向量
//separator 分割符
//keep_empty 是否保留空串，默认为否，不保留
void Separate(const string str,vector<string>& v_str,char separator,bool keep_empty = false);

//分割函数，用于c风格字符串
//cstr 需要分割的字符串
//v_str 存放分割后的子串的向量
//separator 分割符
//keep_empty 是否保留空串，默认为否，不保留
void Separate(const char* cstr,vector<string>& v_str,char separator,bool keep_empty = false);

//正则表达式匹配
bool RegexMatch(const string& str,const string& str_regex);

//XML提取
//待提取字符串 属性名 值存放位置
bool StrXMLGet(const string& str,const string attribute,string& buffer);

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
class TCPClient
{

protected:
    string m_ip;   //服务端地址
    int m_port; //服务端端口
    int m_clientfd; //用于传输的套接字
public:

    TCPClient();
    ~TCPClient();
    bool Connect(std::string in_ip,const unsigned short in_port);
    
    //无头部
    bool Send(const std::string& buffer);
    
    //有头部
    bool Send_h(const std::string& buffer);
    // bool Send(const void * buffer,size_t size);
    
    
    // bool Recv(std::string& buffer,const size_t maxlen);
    // bool Recv(void * buffer,size_t size);

    //无头部
    bool Recv(std::string &buffer,const size_t maxlen,int timeout);
    
    //有头部
    bool Recv_h(std::string &buffer,int timeout);


    //以二进制的形式发送文件
    bool SendFile(const std::string& filename,const string& file_path);
    bool RecvFile(const string& filename,const string& file_path,int file_size);
    bool Close();



};
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
class TCPServer
{
protected:
    //服务器端口
    short m_port;
    //客户端ip
    string m_clientip;
    //监听套接字
    int m_listenfd;
    //传输套接字
    int m_clientfd;
    //客户端ip端口等信息
    struct sockaddr_in m_st_caddr;  

public:
    TCPServer(int client = -1);
    ~TCPServer();
    bool InitServer(const unsigned short in_port);
    bool Accept();
    const string& Clientip() const;
      // 向对端发送报文，成功返回true，失败返回false。
    bool Send(const std::string &buffer);

    bool Send(const void * buffer,size_t size);

    //有头部
    bool Send_h(const std::string& buffer);

  // 接收对端的报文，成功返回true，失败返回false。
  // buffer-存放接收到的报文的内容，maxlen-本次接收报文的最大长度。
    bool Recv(std::string &buffer,const size_t maxlen);

    bool Recv(void * buffer,size_t size);

    bool Recv(std::string &buffer,const size_t maxlen,int timeout);

    //有头部
    bool Recv_h(std::string &buffer,int timeout);

    bool SendFile(const std::string &filename,const string& file_path);
    bool RecvFile(const string& filename,const string& file_path,int file_size);
  // 关闭监听的socket。
    bool CloseListen();

  // 关闭客户端连上来的socket。
    bool CloseClient();

  //返回传输套接字文件描述符
    int GetClientFD();


};

