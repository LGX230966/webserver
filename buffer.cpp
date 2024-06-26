#include "buffer.h"

Buffer::Buffer(uint16_t sep):sep_(sep){}

Buffer::~Buffer(){}

// 把数据追加到buf_中。
void Buffer::Append(const char *data,size_t size)             
{
    buf_.append(data,size);
}

 // 把数据追加到buf_中，附加报文分隔符。
void Buffer::AppendWithSep(const char *data,size_t size)  
{
    if (sep_==0)             // 没有分隔符。
    {
        int remain_size = size;
        int read_pos = 0;
        while(remain_size>0)
        {
            
            int real_size = 4096<remain_size?4096:remain_size;
            buf_.append(data[read_pos],real_size);
            read_pos += real_size;
            remain_size -= real_size;
        }
        
                        // 处理报文内容。
    }
    else if (sep_==1)     // 四字节的报头。
    {
        buf_.append((char*)&size,4);           // 处理报文长度（头部）。
        buf_.append(data,size);                    // 处理报文内容。
    }
    if(sep_ == 2) //http
    {
        int remain_size = size;
        int read_pos = 0;
        while(remain_size>0)
        {
            
            int real_size = 4096<remain_size?4096:remain_size;
            buf_.append(&data[read_pos],real_size);
            read_pos += real_size;
            remain_size -= real_size;
        }
        
    }
}

void Buffer::AppendWithSep(const char *data,size_t size,std::string* strptr)  
{
if (sep_==0)             // 没有分隔符。
{
    int remain_size = size;
    int read_pos = 0;
    while(remain_size>0)
    {
        
        int real_size = 4096<remain_size?4096:remain_size;
        buf_.append(data[read_pos],real_size);
        read_pos += real_size;
        remain_size -= real_size;
    }
    
                    // 处理报文内容。
}
else if (sep_==1)     // 四字节的报头。
{
    buf_.append((char*)&size,4);           // 处理报文长度（头部）。
    buf_.append(data,size);                    // 处理报文内容。
}
if(sep_ == 2)
{
    int remain_size = size;
    int read_pos = 0;
    while(remain_size>0)
    {
        
        int real_size = 4096<remain_size?4096:remain_size;
        buf_.append(&data[read_pos],real_size);
        read_pos += real_size;
        remain_size -= real_size;

    }  
}
delete strptr;
}

// 从buf_的pos开始，删除nn个字节，pos从0开始。
void Buffer::Erase(size_t pos,size_t nn)                             
{
    buf_.erase(pos,nn);
}

// 返回buf_的大小。
size_t Buffer::Size()                                                            
{
    return buf_.size();
}

// 返回buf_的首地址。
const char *Buffer::Data()                                                  
{
    return buf_.data();
}

// 清空buf_。
void Buffer::Clear()                                                            
{
    buf_.clear();
}

// 从buf_中拆分出一个报文，存放在ss中，如果buf_中没有报文，返回false。
bool Buffer::PickMessage(std::string &ss)                           
{
    if (buf_.size()==0) return false;

    if (sep_==0)                  // 没有分隔符。
    {
        ss += buf_;
        buf_.clear();
    }
    else if (sep_==1)          // 四字节的报头。
    {
        int len;
        memcpy(&len,buf_.data(),4);             // 从buf_中获取报文头部。

        if (buf_.size()<len+4) return false;     // 如果buf_中的数据量小于报文头部，说明buf_中的报文内容不完整。

        ss += buf_.substr(4,len);                        // 从buf_中获取一个报文。
        buf_.erase(0,len+4);                          // 从buf_中删除刚才已获取的报文。
    }
    else if(sep_ == 2)
    {   
        // int pos = buf_.find("\r\n\r\n");
        // if(pos == std::string::npos) return true;
        ss += buf_;
        buf_.erase();

    }
    return true;
}

