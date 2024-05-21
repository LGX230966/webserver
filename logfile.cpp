#include"logfile.h"

std::mutex Logfile::mtx_;
Logfile* Logfile::object_ptr_ = nullptr;

Logfile::~Logfile()
{
    fout_.close();
}

Logfile* Logfile::GetObject()
{
    if(object_ptr_ == nullptr)
    {
        std::lock_guard<std::mutex> lg(mtx_);
        if(object_ptr_ == nullptr)
        {
            object_ptr_ = new Logfile();
            object_ptr_->Open("./log.txt");
        }
    }

    return object_ptr_;        

}

void Logfile::Open(const char* filename,std::ios::openmode mode)
{
    if(fout_.is_open())
    {
        fout_.close();
    }
    MakeDir(filename,true);
    fout_.open(filename,mode);
    fout_<<std::unitbuf;
}
std::string Logfile::GetFilename()
{
    std::string str;
    str.assign(filename_,strlen(filename_));//strlen不包括\0
    return str;
}

void Logfile::UpdateTime()
{
    tm_ = localtime(&time_);
    char template_time[] = "%04d-%02d-%02d %02d:%02d:%02d";
    sprintf(str_time_,template_time,tm_->tm_year+1900,tm_->tm_mon+1,tm_->tm_mday,
            tm_->tm_hour,tm_->tm_min,tm_->tm_sec);
}

bool Logfile::MakeDir(const std::string& path_or_filename,bool b_is_filename)
{
     // /tmp/aaa/bbb/ccc/ddd    /tmp    /tmp/aaa    /tmp/aaa/bbb    /tmp/aaa/bbb/ccc 
     
    // 检查目录是否存在，如果不存在，逐级创建子目录
    int pos=1;          // 不要从0开始，0是根目录/。

    while (true)
    {
        int pos1=path_or_filename.find('/',pos);
        if (pos1==std::string::npos) break;

        std::string strpathname=path_or_filename.substr(0,pos1);      // 截取目录。

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

Logfile& Logfile::operator<<(const std::string& str)
{
    {
        std::lock_guard<std::mutex> lg(write_mtx_);
        fout_.write(str.c_str(),str.length());
    }
    
    return *this;
}
