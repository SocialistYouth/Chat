#include "chat/MySQLConnection.h"
#include "chat/chatserver.h"
#include "chat/protocol.h"
#include "sylar/log.h"

mysql::CMySql m_sql;
sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

void dealChatRq(const char *buf) {
    chat::STRU_CHAT_RQ *rq = (chat::STRU_CHAT_RQ *)buf;
    std::cout << "buf: " << rq->content << std::endl;
    std::stringstream ss;
    ss << "insert into `t_message_repository`(`messageType`, `senderUserId`, `receiverUserId`, `messageContent`) VALUES("
       << "'文本', " << rq->userid << ", " << rq->friendid << ", "
       << "'" << rq->content << "'"
       << ");";
    if (!m_sql.UpdateMySql(ss.str().c_str())) {
        SYLAR_LOG_ERROR(g_logger) << "UpdateMySQL Error:" << ss.str();
    }
    ss.str("");
    ss.clear();
    ss << "INSERT INTO `t_message_synchronization`(`messageType`, `sendTo`, `sendFrom`, `messageContent`) VALUES("
       << "'文本', " << rq->friendid << ", " << rq->userid << ", '" << rq->content << "');";
    if(!m_sql.UpdateMySql(ss.str().c_str())) {
        SYLAR_LOG_ERROR(g_logger) << "UpdateMySQL Error:" << ss.str();
    }
    ss.str("");
    ss.clear();
}

void push() {
    std::stringstream ss;
    ss << "SELECT `senderUserId`, `receiverUserId`, `messageContent` FROM `t_message_repository` WHERE `createTime` >= '2023-5-23 14:30:00';";
    std::list<std::string> lstRes;
    if (!m_sql.SelectMySql(ss.str().c_str(), 6, lstRes)) {
        SYLAR_LOG_ERROR(g_logger) << "SelectMySql Error:" << ss.str();
    }
    while (lstRes.size() > 0) {
        int sender = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        int receiver = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        std::string rq = lstRes.front().c_str();
        lstRes.pop_front();
        std::cout << "sender: " << sender << "receiver: " << receiver << "content: " << rq << std::endl;
    }
}

bool updateOfflineTime(int userid) {
    std::stringstream ss;
    ss << "UPDATE `t_user` SET `last_login_time` = NOW() WHERE `uuid` = "
       << 3 << ";";
    if (!m_sql.UpdateMySql(ss.str().c_str())) {
        SYLAR_LOG_ERROR(g_logger) << "UpdateMysql Error:" << ss.str();
    }
    ss.str("");
    ss.clear();
    return true;
}

int main() {
    if (!m_sql.ConnectMySql("127.0.0.1", "cxj", "Becauseofyou0926!", "IM")) {
        SYLAR_LOG_ERROR(g_logger) << "数据库打开失败";
        return 0;
    }
    // chat::STRU_CHAT_RQ rq;
    // rq.friendid = 3;
    // rq.userid   = 4;
    // strcpy(rq.content, "Hello");
    //dealChatRq((char *)&rq);
    // push();
    updateOfflineTime(3);
    return 0;
}