#include <string>
#include <vector>
#include <memory>
struct HTTPPacketInfo
{
    std::string method;
    std::string URL; 
    std::string http_version;
    //头部字段 + 值
    std::vector<std::pair<std::string,std::string>> field_and_value;
    //请求体
    std::vector<std::pair<std::string,std::string>> request_body;
    void Reset()
    {
        method.erase();
        URL.erase();
        http_version.erase();
        field_and_value.clear();
        request_body.clear();
    }
};
/* 主状态机的两种可能状态，分别表示：当前正在分析请求行，当前正在分析请求头,当前正在分析请求体 */
enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_BODY,CHECK_STATE_COMPLETED
};

/* 从状态机的三种可能，即行的读取状态，分别表示：读取到一个完整的行、行出错和行数据尚且不完整 */
enum LINE_STATUS {
    LINE_OK = 0, LINE_BAD, LINE_OPEN
};

/* 服务器处理HTTP请求的结果：
 * NO_REQUEST 表示请求不完整，需要继续读取客户端数据；
 * GET_REQUST 表示获得了一个完整的客户端请求;
 * BAD_REQUST 表示客户端请求语法有问题；
 * FORBIDDEN_REQUEST 表示客户对资源没有足够的访问权限;
 * INTERNAL_ERROR 表示服务器内部错误；
 * CLOSE_CONNECTION 表示客户端已经关闭连接了;
 */
enum HTTP_CODE {
    NO_REQUEST = 0, GET_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSE_CONNECTION
};

class HttpStateMachine
{
private:
    CHECK_STATE _check_state = CHECK_STATE_REQUESTLINE;
    // LINE_STATUS _line_status = LINE_OK;
    // HTTP_CODE _http_code = NO_REQUEST;
    size_t _read_count = 0;    //parse_line读过的字符数
    size_t _parsed_count = 0;
    size_t _size = 0;

    std::string _buffer;
    HTTPPacketInfo _info;//分析过程中报文信息存在这里
    HTTPPacketInfo _last_info;//分析完成后报文信息存在这里，_info会被清空已备下一次使用

    size_t _remain_content_length = 0;//报文告知的请求体剩余长度
    // size_t  = 0;
public:
    HttpStateMachine() ;
    //分析请求头
    LINE_STATUS parse_line();

    /* 分析请求行 */
    HTTP_CODE parse_requestline();

    /* 分析头部字段 */
    HTTP_CODE parse_header();

    /* 分析头部字段 */
    HTTP_CODE parse_body();

    /* 分析HTTP请求的入口函数 */
    HTTP_CODE parse_content();

    void Reset();

    bool Parse();

    HTTPPacketInfo GetInfo();

    void Append(std::string& str);

};
