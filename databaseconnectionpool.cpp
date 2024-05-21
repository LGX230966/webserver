#include "databaseconnectionpool.h"

DatabaseConnectionPool::DatabaseConnectionPool(const std::string& host,const std::string& user,const std::string& db,
const std::string& passwd,unsigned int port,char * unix_socket,unsigned long clientflag):total_amount_(10)
{
    free_amount_ = total_amount_.load();
    for(int i = 0;i < total_amount_;++i)
    {
        MYSQL *mysql = mysql_init(NULL);
        if(mysql_real_connect(mysql,host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
        {
            printf("连接数据库失败\n");
            mysql_close(mysql);
        }
        else
        {
            mysql_set_character_set(mysql, "utf8");
            free_queue_.push(mysql);
            connection_set_.insert(mysql);
        }
    }
}
DatabaseConnectionPool::~DatabaseConnectionPool()
{
    for(auto &ii:connection_set_)
    {
        mysql_close(ii);
    }
}
DatabaseConnectionPool* DatabaseConnectionPool::GetInstance()
{
    static DatabaseConnectionPool db_conn_pool;
    return &db_conn_pool;
    
}
//从空闲队列中取出一个空闲连接
MYSQL* DatabaseConnectionPool:: GetConnection()
{
    if(connection_set_.empty())
    {
        return nullptr;
    }
    std::unique_lock<std::mutex> lock(mtx_);
    if(connection_set_.empty())
    {
        return nullptr;
    }
    /*
    std::condition_variable 提供了两种 wait() 函数。
    当前线程调用 wait() 后将被阻塞(此时当前线程应该获得了锁（mutex），不妨设获得锁 lck)，
    直到另外某个线程调用 notify_* 唤醒了当前线程，该函数会自动调用 lck.unlock() 释放锁，
    使得其他被阻塞在锁竞争上的线程得以继续执行。
    在第二种情况下（即设置了 Predicate），只有当 pred 条件为 false 时调用 wait() 才会阻塞当前线程，
    并且在收到其他线程的通知后只有当 pred 为 true 时才会被解除阻塞。
    */
    condition_.wait(lock,[this]{
        return !free_queue_.empty();
    });
    MYSQL* temp = free_queue_.front();
    free_queue_.pop();
    return temp;
}
bool DatabaseConnectionPool:: ReleaseConnection(MYSQL* mysql)
{
    if(connection_set_.count(mysql) == 1)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        free_queue_.push(mysql);
        condition_.notify_one();
        return true;
    }
    
    return false;
}
