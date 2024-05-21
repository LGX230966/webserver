#include "mylib.h"
bool DirOpe::OpenDir(const string& dir_path,int model)
{
    if(access(dir_path.c_str(),F_OK) == -1)
    {
        if(model == READ)
            return false;
        if(model == CREATE)
            if(MkDir(dir_path,false) == false)
                return false;
    }
    if((m_dir = opendir(dir_path.c_str())) == NULL)
        return false;
    
    return true;
}

//绝对路径 处理函数(绝对路径，可变参数列表) 可变参数列表
bool DirOpe::MkDir(const string& path_or_filename,bool b_is_filename)
{
    size_t find_pos = 1;          // 不要从0开始，0是根目录/。

    while (true)
    {
        size_t substr_pos;

        if((substr_pos = path_or_filename.find("/",find_pos)) == string::npos)
            break;
        //  / r o o t / c o d e / l o g . t x t 
        //  0 1 2 3 4 5 6 7 8 9
        string path(path_or_filename.substr(0,substr_pos));
        if(access(path.c_str(),F_OK) == -1)
            if(mkdir(path.c_str(),0755) == -1)
                return false;

        find_pos = substr_pos + 1;
    }

    // 如果path_or_filename不是文件，是目录，还需要创建最后一级子目录。
    if (b_is_filename==false)
    {
        if (access(path_or_filename.c_str(),F_OK) == -1)
        {
            if (mkdir(path_or_filename.c_str(),0755) == -1) return false;
        }
    }

    return true;
}


//绝对路径 处理函数(绝对路径，可变参数列表) 可变参数列表(回调函数需要的参数)
//void(*p)(const string& 文件的路径,va_list 回调函数需要的参数)
//回调函数需要把valist复制一份，用副本
bool DirOpe::Traverse(const string& dir_path ,bool include_subdir,void(*p)(const string&,va_list valist),...)
{
    va_list valist;
    va_start(valist, p);
    // va_list valist_copy;//副本用于递归传参，否则每个参数只能获取一遍，无法递归
    // va_copy(valist_copy,valist);

    DIR *dir;
    struct dirent* st_entry;

    if((dir = opendir(dir_path.c_str())) == NULL)
        return false;

    while ((st_entry = readdir(dir)) != NULL)
    {
        if(strcmp(st_entry->d_name,".") == 0 || strcmp(st_entry->d_name,"..") == 0)
            continue;
        if(st_entry->d_type == DT_DIR)
        {
            if(include_subdir == false)
                continue;
            Traverse(dir_path + "/" + st_entry->d_name,include_subdir,p,valist);
        }
        else if(st_entry->d_type == DT_REG)
        {
            p(dir_path + "/" + st_entry->d_name,valist);
        }
            
    }
    va_end(valist);
    // va_end(valist_copy);
    return true;
    
}

bool DirOpe::Traverse(const string& dir_path,bool include_subdir,void(*p)(const string&,va_list valist),va_list valist)
{
    // va_list valist_copy;//副本用于递归传参，否则每个参数只能获取一遍，无法递归
    // va_copy(valist_copy,valist);

    DIR *dir;
    struct dirent* st_entry;

    if((dir = opendir(dir_path.c_str())) == NULL)
        return false;

    while ((st_entry = readdir(dir)) != NULL)
    {
        if(strcmp(st_entry->d_name,".") == 0 || strcmp(st_entry->d_name,"..") == 0)
            continue;
        if(st_entry->d_type == DT_DIR)
        {
            if(include_subdir == false)
                continue;
            Traverse(dir_path + "/" + st_entry->d_name,include_subdir,p,valist);
        }
        else if(st_entry->d_type == DT_REG)
        {
            p(dir_path + "/" + st_entry->d_name,valist);
        }
            
    }
    va_end(valist);
    // va_end(valist_copy);
    return true;

}


void FileIO::ReadLine(string& buffer)
{
    getline(fio,buffer);
}
//读取1000个字符
void FileIO::Read(string& buffer)
{
    char cbuffer[1001];
    memset(cbuffer,0,sizeof(cbuffer));
    fio.read(cbuffer,1000);
    buffer = cbuffer;
}
//写一行
void FileIO::WriteLine(string& buffer)
{
    fio.write(buffer.c_str(),strlen(buffer.c_str()));
    fio.write("\n",strlen("\n"));
}
//有多少写多少
void FileIO::Write(string& buffer)
{
    fio.write(buffer.c_str(),strlen(buffer.c_str()));
}
//用于创建目录
bool FileIO::MakeDir(const string& path_or_filename,bool b_is_filename)
{
     // /tmp/aaa/bbb/ccc/ddd    /tmp    /tmp/aaa    /tmp/aaa/bbb    /tmp/aaa/bbb/ccc 
     
    // 检查目录是否存在，如果不存在，逐级创建子目录
    int pos=1;          // 不要从0开始，0是根目录/。

    while (true)
    {
        int pos1=path_or_filename.find('/',pos);
        if (pos1==string::npos) break;

        string strpathname=path_or_filename.substr(0,pos1);      // 截取目录。

        pos=pos1+1;       // 位置后移。
        if (access(strpathname.c_str(),F_OK) != 0)  // 如果目录不存在，创建它。
        {
            // 0755是八进制，不要写成755。
            if (mkdir(strpathname.c_str(),0755) != 0) return false;  // 如果目录不存在，创建它。
        }
    }

    // 如果path_or_filename不是文件，是目录，还需要创建最后一级子目录。
    if (b_is_filename==false)
    {
        if (access(path_or_filename.c_str(),F_OK) != 0)
        {
            if (mkdir(path_or_filename.c_str(),0755) != 0) return false;
        }
    }

    return true;
}

//打开文件，若没有文件则创建文件
bool FileIO::Open(const char* file_path,std::ios::openmode mode)
{
    Close();
    if(access(file_path,F_OK) != 0)
    {
        if(MakeDir(file_path,true) == false) return false;
    }
    fio.open(file_path,mode);
    fio<<std::unitbuf;
    return true;
}

void FileIO::Close()
{
    if(fio.is_open())   fio.close();
}

bool FileIO::XMLGet(const string attribute,string& buffer)
{
    // fio.clear();
    fio.seekg(0,std::ios::beg);
    char cbuffer[1001]; //每次读1000个字符
    memset(cbuffer,0,sizeof(cbuffer));
    while(fio.eof() == false)
    {
        fio.read(cbuffer,1000);
        if(StrXMLGet(cbuffer,attribute,buffer)) return true;
    }
    return false;
}
//判断是否读到文件底
bool FileIO::_EOF()
{
    return fio.eof();
}


// Logfile::~Logfile()
// {
//     m_fout.close();
// }

// void Logfile::Open(const char* filename,ios::openmode mode)
// {
//     if(m_fout.is_open())
//     {
//         m_fout.close();
//     }
//     MakeDir(filename,true);
//     m_fout.open(filename,mode);
//     m_fout<<unitbuf;
// }
// string Logfile::GetFilename()
// {
//     string str;
//     str.assign(m_filename,strlen(m_filename));//strlen不包括\0
//     return str;
// }

// void Logfile::UpdateTime()
// {
//     m_st_tm = localtime(&m_time);
//     char template_time[] = "%04d-%02d-%02d %02d:%02d:%02d";
//     sprintf(m_cstr_time,template_time,m_st_tm->tm_year+1900,m_st_tm->tm_mon+1,m_st_tm->tm_mday,
//             m_st_tm->tm_hour,m_st_tm->tm_min,m_st_tm->tm_sec);
// }

// bool Logfile::MakeDir(const string& path_or_filename,bool b_is_filename)
// {
//      // /tmp/aaa/bbb/ccc/ddd    /tmp    /tmp/aaa    /tmp/aaa/bbb    /tmp/aaa/bbb/ccc 
     
//     // 检查目录是否存在，如果不存在，逐级创建子目录
//     int pos=1;          // 不要从0开始，0是根目录/。

//     while (true)
//     {
//         int pos1=path_or_filename.find('/',pos);
//         if (pos1==string::npos) break;

//         string strpathname=path_or_filename.substr(0,pos1);      // 截取目录。

//         pos=pos1+1;       // 位置后移。
//         if (access(strpathname.c_str(),F_OK) != 0)  // 如果目录不存在，创建它。
//         {
//             // 0755是八进制，不要写成755。
//             if (mkdir(strpathname.c_str(),0755) != 0) return false;  // 如果目录不存在，创建它。
//         }
//     }

//     // 如果path_or_filename不是文件，是目录，还需要创建最后一级子目录。
//     if (b_is_filename==false)
//     {
//         if (access(path_or_filename.c_str(),F_OK) != 0)
//         {
//             if (mkdir(path_or_filename.c_str(),0755) != 0) return false;
//         }
//     }

//     return true;
// }

// Logfile& Logfile::operator<<(const string& str)
// {
//     m_fout.write(str.c_str(),str.length());
//     return *this;
// }


/////////////////////////////////////////////////////////////////////////////
Sem::Sem():m_key(-1),m_semid(-1)
{
}

Sem::~Sem()
{
    Close();
}
bool Sem::Init(key_t key,int val)
{
    m_key = key;
    if(m_semid != -1) return false;
    if((m_semid = semget(m_key,1,0666)) == -1)
    {
        if((m_semid = semget(m_key,1,0666|IPC_CREAT)) == -1)
        {
            std::cout<<"获取信号量失败\n";
            return false;
        }
        if(SetVal(val) == false) return false;
    }
     
    return true;
}

bool Sem::Wait()
{
    if(m_semid == -1)
        return false;
    m_st_sem_opa.sem_num = 0;
    m_st_sem_opa.sem_op = -1;
    m_st_sem_opa.sem_flg = SEM_UNDO;//使操作系统跟踪信号，并在进程没有释放该信号量而终止时，操作系统释放信号量
    if(semop(m_semid,&m_st_sem_opa,1) == -1)
        return false;
    return true;
}

bool Sem::Signal()
{
    if(m_semid == -1)
        return false;
    m_st_sem_opa.sem_num = 0;
    m_st_sem_opa.sem_op =  1;
    m_st_sem_opa.sem_flg = SEM_UNDO;//使操作系统跟踪信号，并在进程没有释放该信号量而终止时，操作系统释放信号量
    if(semop(m_semid,&m_st_sem_opa,1) == -1)
        return false;
    return true;
}

bool Sem::SetVal(int val)
{
    if(m_semid == -1)
    return false;
    un_semun.val = val;
    if(semctl(m_semid,0,SETVAL,un_semun) == -1)
        return false;
    return true;
}

int Sem::GetVal()
{
    if(m_semid == -1)
    return -1;
    return semctl(m_semid,0,GETVAL);
}

bool Sem::Close()
{
    if(m_semid == -1)
        return false;
    if(semctl(m_semid,0,IPC_RMID) == -1)
        return false;
    return true;
}




Pactive::Pactive():m_shmid(-1),m_shm(NULL),m_pos(-1)
{

}
void Pactive::EXIT(int sig)
{
    printf("sig = %d\n",sig);
    if(m_pos != -1)
    {
        sem.Wait();
        memset(m_shm+m_pos,0,sizeof(procinfo));
        sem.Signal();  // 将共享内存中的结构体内容置0
    }

    if(m_shm != NULL)       //解除共享内存与进程的连接
        shmdt(m_shm); 
                            
        exit(0);
}

void Pactive::Init(string p_name,int act_cycle,time_t timeout,key_t key,key_t sem_key)
{
    //信号量初始化
    sem.Init(sem_key);

    time_t m_timeout;

     if((m_shmid = shmget(key,100*sizeof(procinfo),0666)) == -1)
    {
        std::cout<<"获取共享内存("<<key<<")失败\n";
        EXIT(-1);
    }

    if((m_shm = (procinfo*)shmat(m_shmid, 0, 0)) == (void*)-1)
    {
        std::cout<<"共享内存连接到当前进程失败\n";
        EXIT(-1);
    }


    //载入进程信息
    procinfo st_procinfo(getpid(),p_name.c_str(),timeout,time(0));

    sem.Wait();//申请信号量
    for(int ii = 0; ii < 100; ii++)
    {
        cout<<ii<<":"<<m_shm[ii].pid<<endl;
        if(m_shm[ii].pid == st_procinfo.pid)
        {
            m_pos = ii;
            printf("找到旧位置ii=%d\n",ii);
            break;
        }
    }
    if(m_pos == -1)
    {  
        for(int ii = 0; ii < 100; ii++)
        {
            if((m_shm[ii]).pid == 0)
            {
                m_pos = ii;
                printf("找到新位置ii=%d\n",ii);
                break;
            }
        }
    }
    if(m_pos == -1)
    {

        printf("共享内存已满\n");
        sem.Signal();
        EXIT(-1);
    }

    memcpy(m_shm+m_pos,&st_procinfo,sizeof(procinfo));
    sem.Signal();//释放信号量


    while(1)
    {
        sleep(act_cycle);

        sem.Wait();
        //更新活动时间
        (m_shm+m_pos)->atime = time(0);
        sem.Signal();
    }

}

Pactive::~Pactive()
{
    //EXITd调用exit exit调用全局对象的析构函数 两次调用导致段错误
    // EXIT(0);

}

void Separate(const string str,vector<string>& v_str,char separator,bool keep_empty)
{
    string buffer = str;
    size_t pos = 0;
    while((pos = buffer.find(separator))!= string::npos)
    {
        v_str.push_back(buffer.substr(0,pos));
        buffer = buffer.substr(pos+1);                          // char:    1 2 3 / 4 5 6 7 8 9
                                                                // pos:     0 1 2 3 4 5 6 7 8 9
    }
    v_str.push_back(buffer);//将剩余的字符串加入容器
    if(!keep_empty)//如果不保留空串，将容器中的空串删除
    {
        for(auto it = v_str.begin(); it!=v_str.end(); it++)
        {
            if(*it == "")
                v_str.erase(it);
        }
    }
}

void Separate(const char* cstr,vector<string>& v_str,char separator,bool keep_empty)
{
    char sep[2];
    memset(sep,0,sizeof(sep));
    sep[0] = separator;
    char buffer[strlen(cstr)+1];//不能用sizeof sizeof(cstr)是指针大小，八字节
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,cstr);

    char* pos = NULL;
    while((pos = strstr(buffer,sep)) != NULL)
    {
        char subcstr[(int)(pos-buffer)+1];
        memset(subcstr,0,sizeof(subcstr)); 
        strncpy(subcstr,buffer,(int)(pos-buffer));
        v_str.push_back(subcstr);
        strcpy(buffer,buffer+(int)(pos-buffer+1));                  // char:    1 2 3 / 4 5 6 7 8 9
                                                                    // pos:     0 1 2 3 4 5 6 7 8 9
    }
    v_str.push_back(buffer);//将剩余的字符串加入容器
    if(!keep_empty)//如果不保留空串，将容器中的空串删除
    {
        for(auto it = v_str.begin(); it!=v_str.end(); it++)
        {
            if(*it == "")
                v_str.erase(it);
        }
    }
}

bool RegexMatch(const string& str,const string& str_regex)
{
    regex pattern(str_regex);
    return regex_match(str,pattern);
}

bool StrXMLGet(const string& str,const string attribute,string& buffer)
{
    int length = attribute.length();
    int fpos = -1;  //前指针
    int bpos = -1;  //后指针
    //<attr>xxx</attr>
    if((fpos = str.find("<" + attribute + ">")) == string::npos) return false;
    fpos += length + 2;

    if((bpos = str.find("</" + attribute + ">")) == string::npos) return false;
    //取值
    buffer = str.substr(fpos,bpos - fpos);
    return true;
}

TCPClient::TCPClient():m_clientfd(-1){}

bool TCPClient::Connect(std::string in_ip,const unsigned short in_port)
{
    m_ip = in_ip;
    m_port = in_port;
    m_clientfd = socket(PF_INET,SOCK_STREAM,0);
    if(m_clientfd == -1)
    {
        perror("clientfd:");
        return false;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    struct hostent *h = nullptr;
    if((h = gethostbyname(in_ip.c_str())) == nullptr)
    {
        perror("gethostbyname:");
        ::close(m_clientfd);
        return false;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_port);
    memcpy(&servaddr.sin_addr,h->h_addr_list[0],h->h_length);

    if(::connect(m_clientfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("connect:");
        ::close(m_clientfd);
        return false;
    }
    return true;
}
bool TCPClient::Send(const std::string& buffer)
{
    if (m_clientfd==-1) return false; // 如果socket的状态是未连接，直接返回失败。

    if ((::send(m_clientfd,buffer.data(),buffer.size(),0))<=0) return false;
    
    return true;
}
bool TCPClient::Send_h(const std::string& buffer)
{
    if (m_clientfd==-1) return false; // 如果socket的状态是未连接，直接返回失败。
    
    uint32_t size = htonl(buffer.length());
    if ((::send(m_clientfd,(char*)&size,sizeof(uint32_t),0))<=0) return false;
    if ((::send(m_clientfd,buffer.c_str(),buffer.length(),0))<=0) return false;
    
    return true;
}

// bool TCPClient::Send(const void * buffer,size_t size)
// {
//     if (m_clientfd==-1) return false; // 如果socket的状态是未连接，直接返回失败。
//     if ((::send(m_clientfd,(char*)htonl(size),sizeof(uint32_t),0))<=0) return false;
//     int rt = ::send(m_clientfd,buffer,size,0);
//     if(rt == -1) return false;
//     return true;
// }

// bool TCPClient::Recv(std::string &buffer,const size_t maxlen)
// {
    
//     buffer.clear();         // 清空容器。
//     buffer.resize(sizeof(uint32_t));  // 设置容器的大小为maxlen。
//     if ((::recv(m_clientfd,&buffer[0],buffer.size(),0))<=0) return false;
//     int size = atoi(buffer.c_str());
//     buffer.clear();         // 清空容器。
//     buffer.resize(size);
//     int readn=::recv(m_clientfd,&buffer[0],size,0);  // 直接操作buffer的内存。
//     if (readn<=0) { buffer.clear(); return false; }
//     buffer.resize(readn);   // 重置buffer的实际大小。

//     return true;
// }

// bool TCPClient::Recv(void * buffer,size_t size)
// {
//     int rt = ::recv(m_clientfd,buffer,size,0);
//     if(rt == -1) return false;
//     return true;
// }

//有头部
bool TCPClient::Recv_h(std::string &buffer,int timeout)
{
    // int select(int maxfd, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);
//     struct timeval
//     {      
// 	       long tv_sec;   /*秒 */
// ​	    long tv_usec;  /*微秒 */   
//     }
    struct timeval* st_timeout = NULL;
    if(timeout >= 0) 
    {
        st_timeout = (struct timeval*)malloc(sizeof(struct timeval));
        st_timeout->tv_sec = timeout;
        st_timeout->tv_usec = 0;
    }
    
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(m_clientfd,&readset);
    fd_set readset_bak1 = readset;
    fd_set readset_bak2 = readset;
    uint32_t size = 0;
    if(select(m_clientfd+1,&readset_bak1,nullptr,nullptr,st_timeout) > 0)
    {
        buffer.clear();         // 清空容器。
        buffer.resize(sizeof(uint32_t));  // 设置容器的大小为maxlen。
        int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
        if (readn<=0) { buffer.clear();free(st_timeout); return false; }
        size = ntohl(*(reinterpret_cast<uint32_t*>(const_cast<char*>(buffer.c_str()))));
    }
    if(select(m_clientfd+1,&readset_bak2,nullptr,nullptr,st_timeout) > 0)
    {
        buffer.clear();         // 清空容器。
        buffer.resize(size);  // 设置容器的大小为maxlen。
        int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
        if (readn<=0) { buffer.clear(); free(st_timeout);return false; }
        buffer.resize(readn);   // 重置buffer的实际大小。
        free(st_timeout);
        return true;
    }
    free(st_timeout);
    return false;

}
//无头部
bool TCPClient::Recv(std::string &buffer,const size_t maxlen,int timeout)
{
    // int select(int maxfd, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);
//     struct timeval
//     {      
// 	       long tv_sec;   /*秒 */
// ​	    long tv_usec;  /*微秒 */   
//     }
    struct timeval* st_timeout = NULL;
    if(timeout >= 0) 
    {
        st_timeout = (struct timeval*)malloc(sizeof(struct timeval));
        st_timeout->tv_sec = timeout;
        st_timeout->tv_usec = 0;
    }
    
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(m_clientfd,&readset);
    if(select(m_clientfd+1,&readset,nullptr,nullptr,st_timeout) > 0)
    {
        buffer.clear();         // 清空容器。
        buffer.resize(maxlen);  // 设置容器的大小为maxlen。
        int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
        if (readn<=0) { buffer.clear(); free(st_timeout);return false; }
        buffer.resize(readn);   // 重置buffer的实际大小。
        free(st_timeout);
        return true;
    }
    free(st_timeout);
    return false;

}
bool TCPClient::Close()
  {
    if (m_clientfd==-1) return false; // 如果socket的状态是未连接，直接返回失败。

    ::close(m_clientfd);
    m_clientfd=-1;
    return true;
  }


//以二进制的形式发送文件
bool TCPClient::SendFile(const std::string &filename,const string& file_path)
{
    FileIO fio;
    FileIO::MakeDir(file_path,false);
    if(fio.Open((file_path + "/" + filename).c_str(),std::ios::in|std::ios::binary) == false)    return false;
    string buffer;
    while(fio._EOF() == false)
    {
        fio.Read(buffer);
        // cout<<buffer<<endl;
        Send(buffer);
    }
    return true;
}
bool TCPClient::RecvFile(const string& filename,const string& file_path,int file_size)
{
    FileIO fio;
    FileIO::MakeDir(file_path,false);
    if(fio.Open((file_path + "/" + filename).c_str(),std::ios::out|std::ios::binary) == false)    return false;
    int remain = file_size;
    string buffer;
    while(remain > 0)
    {
        // cout<<received<<endl;
        Recv(buffer,remain < 1000 ? remain : 1000,10);
        remain -= buffer.length();  
        fio.Write(buffer);
    }
    return true; 
}
TCPClient::~TCPClient()
{
    Close();
}
///////////////////////////////////////////////////////////////////////////////////
TCPServer::TCPServer(int clientfd):m_listenfd(-1),m_clientfd(clientfd){}

bool TCPServer::InitServer(const unsigned short in_port)
{
    m_port = in_port;
    //确定ipv4/ipv6 tcp/udp
    m_listenfd = socket(PF_INET,SOCK_STREAM,0);
    if(m_listenfd == -1)
    {
        perror("socket:");
        return -1;
    }
    struct sockaddr_in servAddr;
    memset(&servAddr,0,sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port=htons(in_port);
    //在bind之前
    //端口复用，没懂
    int opt = 1;
    setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    
    //服务端ip端口
    if(bind(m_listenfd,(struct sockaddr*)&servAddr,sizeof(servAddr)) == -1)
    {
        perror("bind:");
        close(m_listenfd);
        return false;
    }
    if(listen(m_listenfd,100) == -1)
    {
        perror("listen:");
        close(m_listenfd);
        return false;
    }
    // m_clientfd = ::accept(m_listenfd,0,0);
    // if (m_clientfd == -1)
    // {
    //     perror("accept"); close(m_listenfd); return -1; 
    // }
    // std::cout << "客户端已连接。\n";
    return true;
}

bool TCPServer::Accept()
{      // 客户端的地址信息。  
    socklen_t addrlen=sizeof(m_st_caddr); // struct sockaddr_in的大小。
    if ((m_clientfd=::accept(m_listenfd,(struct sockaddr *)&m_st_caddr,&addrlen))==-1) return false;
    std::cout<<"客户端已连接"<<std::endl;
    m_clientip=inet_ntoa(m_st_caddr.sin_addr);  // 把客户端的地址从大端序转换成字符串。

    return true;
}

  // 获取客户端的IP(字符串格式)。
const std::string & TCPServer::Clientip() const
{
    return m_clientip;
}

  // 向对端发送报文，成功返回true，失败返回false。
bool TCPServer::Send(const std::string &buffer)   
{
    if (m_clientfd==-1) return false;

    if ( (::send(m_clientfd,buffer.data(),buffer.size(),0))<=0) return false;
   
    return true;
}

bool TCPServer::Send(const void * buffer,size_t size)
{
    if (m_clientfd==-1) return false; // 如果socket的状态是未连接，直接返回失败。
    int rt = ::send(m_clientfd,buffer,size,0);
    if(rt == -1) return false;
    return true;
}

bool TCPServer::Send_h(const std::string& buffer)
{
    if (m_clientfd==-1) return false; // 如果socket的状态是未连接，直接返回失败。
    uint32_t size = htonl(buffer.length());
    if ((::send(m_clientfd,(char*)&size,sizeof(uint32_t),0))<=0) return false;
    if ((::send(m_clientfd,buffer.c_str(),buffer.length(),0))<=0) return false;
    
    return true;
}
  // 接收对端的报文，成功返回true，失败返回false。
  // buffer-存放接收到的报文的内容，maxlen-本次接收报文的最大长度。
bool TCPServer::Recv(std::string &buffer,const size_t maxlen)
{ 
    buffer.clear();         // 清空容器。
    buffer.resize(maxlen);  // 设置容器的大小为maxlen。
    int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
    if (readn<=0) { buffer.clear(); return false; }
    buffer.resize(readn);   // 重置buffer的实际大小。
    return true;
}
bool TCPServer::Recv(void * buffer,size_t size)
{

    if(::recv(m_clientfd,buffer,size,0) == -1) return false;
    return true;
}
bool TCPServer::Recv(std::string &buffer,const size_t maxlen,int timeout)
{
    // int select(int maxfd, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);
//     struct timeval
//     {      
// 	       long tv_sec;   /*秒 */
// ​	    long tv_usec;  /*微秒 */   
//     }
    struct timeval* st_timeout = NULL;
    if(timeout >= 0) 
    {
        st_timeout = (struct timeval*)malloc(sizeof(struct timeval));
        st_timeout->tv_sec = timeout;
        st_timeout->tv_usec = 0;
    }
    
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(m_clientfd,&readset);
    if(select(m_clientfd+1,&readset,nullptr,nullptr,st_timeout) > 0)
    {
        buffer.clear();         // 清空容器。
        buffer.resize(maxlen);  // 设置容器的大小为maxlen。
        int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
        if (readn<=0) { buffer.clear(); free(st_timeout);return false; }
        buffer.resize(readn);   // 重置buffer的实际大小。
        free(st_timeout);
        return true;
    }
    free(st_timeout);
    return false;

}
//有头部
bool TCPServer::Recv_h(std::string &buffer,int timeout)
{
    // int select(int maxfd, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);
//     struct timeval
//     {      
// 	       long tv_sec;   /*秒 */
// ​	    long tv_usec;  /*微秒 */   
//     }
    struct timeval* st_timeout = NULL;
    if(timeout >= 0) 
    {
        st_timeout = (struct timeval*)malloc(sizeof(struct timeval));
        st_timeout->tv_sec = timeout;
        st_timeout->tv_usec = 0;
    }
    
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(m_clientfd,&readset);
    fd_set readset_bak1 = readset;
    fd_set readset_bak2 = readset;
    uint32_t size = 0;
    if(select(m_clientfd+1,&readset_bak1,nullptr,nullptr,st_timeout) > 0)
    {
        buffer.clear();         // 清空容器。
        buffer.resize(sizeof(uint32_t));  // 设置容器的大小为maxlen。
        int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
        if (readn<=0) { buffer.clear();free(st_timeout); return false; }
        size = ntohl(*(reinterpret_cast<uint32_t*>(const_cast<char*>(buffer.c_str()))));

    }
    if(select(m_clientfd+1,&readset_bak2,nullptr,nullptr,st_timeout) > 0)
    {
        buffer.clear();         // 清空容器。
        buffer.resize(size);  // 设置容器的大小为maxlen。
        int readn=::recv(m_clientfd,&buffer[0],buffer.size(),0);  // 直接操作buffer的内存。
        if (readn<=0) { buffer.clear(); free(st_timeout);return false; }
        buffer.resize(readn);   // 重置buffer的实际大小。
        free(st_timeout);
        return true;
    }
    free(st_timeout);
    return false;

}
bool TCPServer::SendFile(const std::string &filename,const string& file_path)
{
    FileIO fio;
    fio.Open((file_path + "/" + filename).c_str(),std::ios::in|std::ios::binary);
    string buffer;
    while(fio._EOF() == false)
    {
        fio.Read(buffer);
        Send(buffer);
    }
    return true;
}
bool TCPServer::RecvFile(const string& filename,const string& file_path,int file_size)
{
    FileIO fio;
    FileIO::MakeDir(file_path,false);
    if(fio.Open((file_path + "/" + filename).c_str(),std::ios::out|std::ios::binary) == false)    return false;
    int remain = file_size;
    string buffer;
    while(remain > 0)
    {
        // cout<<received<<endl;
        Recv(buffer,remain < 1000 ? remain : 1000,10);
        remain -= buffer.length();  
        fio.Write(buffer);
    }
    return true;
}

  // 关闭监听的socket。
bool TCPServer::CloseListen()
{
    if (m_listenfd==-1) return false; 

    ::close(m_listenfd);
    m_listenfd=-1;
    return true;
}

  // 关闭客户端连上来的socket。
bool TCPServer::CloseClient()
{
    if (m_clientfd==-1) return false;

    ::close(m_clientfd);
    m_clientfd=-1;
    return true;
}

TCPServer::~TCPServer() 
{ 
    CloseListen(); 
    CloseClient(); 
}

int TCPServer::GetClientFD()
{
    return m_clientfd;
}
