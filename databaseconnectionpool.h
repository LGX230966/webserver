#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <queue>
#include <unordered_set>

class DatabaseConnectionPool{

private:
    // char host_[16];//15 + "\0"
    // char user_[33];//MySQL 用户名最长为 32 个字符
    // char passwd[42];//密码最长41个字符
    // char db_[65];//数据库名、表名、字段长度最大为64个字符
    // unsigned int port_;//默认3306
    // char *unix_socket_;//NULL
    // unsigned long clientflag_ ;//0
    std::condition_variable condition_;
    std::mutex mtx_;
    // std::vector<MYSQL*> connections_;
    std::atomic<int> total_amount_;
    std::atomic<int> free_amount_;
    std::queue<MYSQL*> free_queue_;
    std::unordered_set<MYSQL*> connection_set_;


    DatabaseConnectionPool(const std::string& host = "39.98.55.100",
                        const std::string& user = "root",
                        const std::string& db = "webserver",
                        const std::string& passwd = "XPR87588772",
                        unsigned int port = 3306,
                        char * unix_socket = NULL,
                        unsigned long clientflag = 0);
    ~DatabaseConnectionPool();

public:
    static DatabaseConnectionPool* GetInstance();
    MYSQL* GetConnection();
    bool ReleaseConnection(MYSQL* mysql);

};