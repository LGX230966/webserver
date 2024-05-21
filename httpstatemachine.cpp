#include "httpstatemachine.h"

HttpStateMachine::HttpStateMachine(){}

HTTP_CODE HttpStateMachine::parse_content()
{
    HTTP_CODE ret = NO_REQUEST;
    bool stop = false;
    // printf("1\n");
    while(!stop && _parsed_count < _size)
    {
        // printf("2\n");
        switch(parse_line())
        {
            case LINE_OK:
                break;
            case LINE_OPEN:
                return NO_REQUEST;
            case LINE_BAD:
                Reset(); //报文错误buffer清空，状态重置
                return BAD_REQUEST;
        }
        switch(_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
                ret =  parse_requestline();
                if(ret != NO_REQUEST)
                {
                    Reset();
                    break;
                }
                continue;
            case CHECK_STATE_HEADER:
                ret = parse_header();
                if(ret == GET_REQUEST)
                {
                    Reset();
                    stop = true;
                    break;
                }
                if(ret != NO_REQUEST)
                {
                    Reset();
                    break;
                }
                continue;

            case CHECK_STATE_BODY:
                ret = parse_body();
                if(ret == GET_REQUEST)
                {
                    Reset();
                    stop = true;
                    break;
                }
                if(ret == NO_REQUEST)
                {
                    stop = true;
                    break;
                }
                //除此之外
                Reset();
                stop = true;
                break;
            default:
                Reset();
                printf("bad check_state\n");
        }
    }
    return ret;
}

HTTP_CODE HttpStateMachine::parse_requestline()
{
    //method
    size_t pos = _buffer.find_first_of(" \t",_parsed_count);
    if(pos == std::string::npos || pos >= _read_count)
    {
        _parsed_count = _read_count;
        return BAD_REQUEST;
    }
    _info.method = _buffer.substr(_parsed_count,pos - _parsed_count);
    _parsed_count = pos + 1;
    
    //URL
    pos = _buffer.find_first_of(" \t",_parsed_count);
    if(pos == std::string::npos || pos >= _read_count)
    {
        _parsed_count = _read_count;
        return BAD_REQUEST;
    }
    _info.URL = _buffer.substr(_parsed_count,pos - _parsed_count);
    _parsed_count = pos + 1;
    
    //http_version
    //find_first_of默认情况下无法查找\0字符
    pos = _buffer.find_first_of("\r",_parsed_count);
    if(pos == std::string::npos || pos >= _read_count)
    {
        _parsed_count = _read_count;
        return BAD_REQUEST;
    }
    _info.http_version = _buffer.substr(_parsed_count,pos - _parsed_count);
    _parsed_count = pos + 2;
    _check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HTTP_CODE HttpStateMachine::parse_header()
{
    //空行
    if(_buffer[_parsed_count] == '\r'&&_buffer[_parsed_count + 1] == '\n')
    {
        _parsed_count += 2;
        _check_state = CHECK_STATE_BODY;//这句目前有问题
        if(_info.method == "GET")
        {
            return GET_REQUEST;
        }
        // printf("return NO_REQUEST;\n");
        return NO_REQUEST;
    }
    //:前
    size_t pos = _buffer.find_first_of(":",_parsed_count);
    if(pos == std::string::npos || pos >= _read_count)
    {
        _parsed_count = _read_count;
        return BAD_REQUEST;
    }
    _info.field_and_value.emplace_back(std::pair<std::string,std::string>(_buffer.substr(_parsed_count,pos - _parsed_count),""));
    _parsed_count = pos + 1;
    //:后
    _parsed_count = _buffer.find_first_not_of(" ",_parsed_count);
    pos = _buffer.find_first_of("\r",_parsed_count);
    if(pos == std::string::npos || pos >= _read_count)
    {
        _parsed_count = _read_count;
        return BAD_REQUEST;
    }
    _info.field_and_value.back().second = _buffer.substr(_parsed_count,pos - _parsed_count);
    _parsed_count = pos + 2;
    if(_info.field_and_value.back().first == "Content-Length")
    {
        _remain_content_length = atoi(_info.field_and_value.back().second.c_str());
    }
    return NO_REQUEST;
    
}


LINE_STATUS HttpStateMachine::parse_line()
{
    //处理请求体时，因为请求体结尾没有换行
    if(_check_state == CHECK_STATE_BODY)
    {
        //读完请求体
        if(_remain_content_length <= _size - _read_count)
        {
            _read_count += _remain_content_length;
            _remain_content_length = 0;
            _buffer.insert(_read_count,"\r\n");//手动添加换行符
            _read_count += 2;
            return LINE_OK;
        }
        //没读完
        else
        {
            _read_count = _size;
            _remain_content_length -= _size - _read_count;
            return LINE_OPEN;
        }
    }

    //其他情况
    _read_count = _buffer.find_first_of("\r\n",_read_count);
    if(_read_count == std::string::npos)
    {
        printf("_read_count:npos\n");
        _read_count = _buffer.size();
        return LINE_OPEN;
    }
    //先找到\r看后面是否紧跟\n,如果有则找到完整的一行，如果\r是最后一个字符说明当前行不完整，如果后面不是\n说明当前行错误
    else if(_buffer[_read_count] == '\r')
    {
        if(_read_count < _size - 1)
        {
            if(_buffer[_read_count + 1] == '\n')
            {
                // _buffer[_read_count++] = '\0';
                // _buffer[_read_count++] = '\0';
                _read_count += 2;
                return LINE_OK;
            }
            else
            {
                _read_count += 2;
                return LINE_BAD;
            }
        }
        else
        {
            ++_read_count;
            return LINE_OPEN;
        }
    }
    //先找到\n看前面是否紧挨着\r,如果是说明行完整，否则行不完整
    else if(_buffer[_read_count] == '\n')
    {
        if(_read_count == 0)
        {
            ++_read_count;
            return LINE_BAD;
        }
        else if(_buffer[_read_count - 1] == '\r')
        {
            // _buffer[_read_count - 1] = '\0';
            // _buffer[_read_count++] = '\0';
            ++_read_count;
            return LINE_OK;
        }
    }
    return LINE_BAD;
}
bool HttpStateMachine::Parse()
{
    auto ret = parse_content();
    switch(ret)
    {
        case GET_REQUEST:
            // printf("1\n");
            return true;
        default:
            // printf("2\n");
            return false;
    }
    return false;
}

void HttpStateMachine::Reset()
{
    _buffer.erase(0,_parsed_count);
    _check_state = CHECK_STATE_REQUESTLINE;
    _read_count = _parsed_count = 0;
    _size = _buffer.size();
    _last_info = _info;
    _info.Reset();
    _remain_content_length = 0;
}

void HttpStateMachine::Append(std::string& str)
{  
    _buffer += str;
    _size = _buffer.size();
}

HTTPPacketInfo HttpStateMachine::GetInfo()
{
    return _last_info;
}

HTTP_CODE HttpStateMachine::parse_body()
{
    
    while(true)
    {
        //键
        size_t pos = _buffer.find_first_of("=",_parsed_count);
        if(pos == std::string::npos || pos >= _read_count)
        {
            _parsed_count = _read_count;
            return BAD_REQUEST;
        }
        _info.request_body.emplace_back(std::pair<std::string,std::string>(_buffer.substr(_parsed_count,pos - _parsed_count),""));
        _parsed_count = pos + 1;
        //值
        pos = _buffer.find_first_of("&\r\n",_parsed_count);
        if(pos == std::string::npos || pos >= _read_count)
        {
            _parsed_count = _read_count;
            return BAD_REQUEST;
        }
        _info.request_body.back().second = _buffer.substr(_parsed_count,pos - _parsed_count);
        if(_buffer[pos] == '&')
        {
            
            _parsed_count = pos + 1;
        }
        else if(_buffer[pos] == '\r')
        {
            _parsed_count = pos + 2;
            break;
        }
    }
    return GET_REQUEST;
}