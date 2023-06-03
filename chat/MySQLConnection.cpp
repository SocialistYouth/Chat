#include "MySQLConnection.h"
#include "sylar/log.h"
#include <string.h>

namespace mysql {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");
CMySql::CMySql(void) {
    /*这个函数用来分配或者初始化一个MYSQL对象，用于连接mysql服务端。
    如果你传入的参数是NULL指针，它将自动为你分配一个MYSQL对象，
    如果这个MYSQL对象是它自动分配的，那么在调用mysql_close的时候，会释放这个对象*/
    sock = new MYSQL;
    mysql_init(sock);
    // mysql_ssl_set(sock, NULL, NULL, NULL, NULL, 0);
    /* mysql_set_character_set(sock, "gb2312"); // gb2312 中华人民共和国简体字标准 */
}

CMySql::~CMySql(void) {
    if (sock) {
        delete sock;
        sock = NULL;
    }
}

void CMySql::DisConnect() {
    mysql_close(sock);
}

bool CMySql::ConnectMySql(const char *host, const char *user, const char *pass, const char *db, short nport) {

    if (!mysql_real_connect(sock, host, user, pass, db, nport, NULL, CLIENT_MULTI_STATEMENTS)) {
        // 连接错误
        SYLAR_LOG_ERROR(g_logger) << "Failed to connect to database:Error: " << mysql_error(sock);
        return false;
    }

    return true;
}
bool CMySql::GetTables(const char *szSql, std::list<std::string> &lstStr) {
    if (mysql_query(sock, szSql))
        return false;

    results = mysql_store_result(sock);
    if (NULL == results)
        return false;
    while ((record = mysql_fetch_row(results))) {
        lstStr.push_back(record[0]);
    }

    return true;
}
bool CMySql::SelectMySql(const char *szSql, int nColumn, std::list<std::string> &lstStr) {
    // mysql_query() 函数用于向 MySQL 发送并执行 SQL 语句
    if (mysql_query(sock, szSql)) {
        SYLAR_LOG_INFO(g_logger) << "mysql_query failed " << mysql_error(sock);
        return false;
    }
    /**
     * @brief mysql_store_result 对于成功检索了数据的每个查询(SELECT、SHOW、DESCRIBE、EXPLAIN、CHECK TABLE等)
     * @return
        . CR_COMMANDS_OUT_OF_SYNC 　　以不恰当的顺序执行了命令。
        · CR_OUT_OF_MEMORY 　　		  内存溢出。
        · CR_SERVER_GONE_ERROR 　　   MySQL服务器不可用。
        · CR_SERVER_LOST 　　         在查询过程中，与服务器的连接丢失。
        · CR_UNKNOWN_ERROR 　　		  出现未知错误。
    */
    results = mysql_store_result(sock);
    if (mysql_errno(sock)) {
        SYLAR_LOG_INFO(g_logger) << "mysql_store_result failed " << mysql_error(sock);
        return false;
    }
    if (results == NULL)
        return true;
    nColumn = mysql_num_fields(results); // 查询返回结果集有多少个字段
    // 遍历表中的下一行，取出内容放入record 结果集
    while ((record = mysql_fetch_row(results))) {

        for (int i = 0; i < nColumn; i++) {
            if (!record[i])
                lstStr.push_back("");
            else
                lstStr.push_back(record[i]);
        }
    }
    mysql_free_result(results);
    return true;
}

bool CMySql::UpdateMySql(const char *szSql) {
    SYLAR_LOG_INFO(g_logger) << __func__ << " begin()";
    if (!szSql)
        return false;
    SYLAR_LOG_INFO(g_logger) << __func__ << " middle()";
    if (mysql_query(sock, szSql)) {
        return false;
    }
    SYLAR_LOG_INFO(g_logger) << __func__ << " end()";
    return true;
}
} // namespace mysql
