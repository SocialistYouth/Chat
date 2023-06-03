/*
 * @Author: Cui XiaoJun
 * @Date: 2023-05-13 14:08:41
 * @LastEditTime: 2023-05-13 14:55:33
 * @email: cxj2856801855@gmail.com
 * @github: https://github.com/SocialistYouth/
 */
#include "chatserver.h"
#include <sstream>

namespace chat {
sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");
ChatServer::ChatServer(sylar::IOManager *worker /* = sylar::IOManager::GetThis() */,
                       sylar::IOManager *accept_worker /* = sylar::IOManager::GetThis() */)
    : TcpServer(worker, accept_worker) {
    setProtocolMap();
}
void ChatServer::setProtocolMap() {
#define XX(str, func)                                                                                                        \
    {                                                                                                                        \
        auto call = std::bind(&ChatServer::func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); \
        m_deal_items.insert({str, call});                                                                                    \
    }

    XX(_DEF_PACK_LOGIN_RQ, dealLoginRq);
    XX(_DEF_PACK_REGISTER_RQ, dealRegisterRq);
    XX(_DEF_PACK_CHAT_RQ, dealChatRq);
    XX(_DEF_PACK_ADDFRIEND_RQ, dealAddFriendRq);
    XX(_DEF_PACK_ADDFRIEND_RS, dealAddFriendRs);
    XX(_DEF_PROTOCOL_GETUSERINFO_RQ, DealGetUserInfoRq);
    XX(_DEF_PROTOCOL_FILE_INFO_RQ, DealFileInfoRq);
    XX(_DEF_PROTOCOL_FILE_INFO_RS, DealFileInfoRs);
    XX(_DEF_PROTOCOL_FILE_BLOCK_RQ, DealFileBlockRq);
    XX(_DEF_PROTOCOL_FILE_BLOCK_RS, DealFileBlockRs);
    XX(_DEF_PACK_OFFLINE_RQ, DealOfflineRq);

#undef XX
}
void ChatServer::init() {
    // 1. 初始化数据库
    if (!m_sql.ConnectMySql("127.0.0.1", "cxj", "Becauseofyou0926!", "IM")) {
        SYLAR_LOG_ERROR(g_logger) << "数据库打开失败";
        return;
    }
    // 2. 初始化网络
    auto addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8080");
    SYLAR_ASSERT(addr);
    std::vector<sylar::Address::ptr> addrs;
    addrs.push_back(addr);
    std::vector<sylar::Address::ptr> fails;
    while (!this->bind(addrs, fails)) {
        sleep(2);
    }
    SYLAR_LOG_INFO(g_logger) << "bind success, " << this->toString();
    this->start();
}

void ChatServer::handleClient(sylar::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "new client: " << client->toString();
    int nPackSize = 0; // 存储包大小
    int rt        = 0; // 一次接受到的字节数
    while (true) {
        rt = client->recv((char *)&nPackSize, sizeof(int));
        SYLAR_LOG_INFO(g_logger) << "收到包大小:" << nPackSize;
        if (rt <= 0 && nPackSize > 100000) {
            SYLAR_LOG_ERROR(g_logger) << "client error rt=" << rt
                                      << " nPackSize:" << nPackSize << " errno=" << errno << " errstr=" << strerror(errno);
            client->close(); // 网络攻击
            break;
        }
        int offset = 0; // 从buf开始偏移多少
        std::string recvbuf;
        recvbuf.resize(nPackSize);
        // char *recvbuf = new char[nPackSize];
        while (nPackSize) {
            if ((rt = client->recv(&recvbuf[0] + offset, nPackSize)) > 0) {
                SYLAR_LOG_INFO(g_logger) << "接受到一次包大小:" << rt;
                nPackSize -= rt;
                offset += rt;
            }
        }
        recvbuf[offset] = '\0';
        SYLAR_LOG_INFO(g_logger) << "recv: " << recvbuf;
        dealData(client, recvbuf.c_str(), offset);
        if (rt == 0) {
            SYLAR_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        } else if (rt < 0) {
            SYLAR_LOG_INFO(g_logger) << "client error rt=" << rt
                                     << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
    }
}

void ChatServer::dealData(sylar::Socket::ptr client, const char *buf, int buflen) {
    int header_type = *(int *)buf;
    SYLAR_LOG_INFO(g_logger) << "DealData() header_type:" << header_type;
    if (header_type >= _DEF_PROTOCOL_BASE && header_type <= _DEF_PROTOCOL_BASE + _DEF_PROTOCOL_COUNT)
        m_deal_items[header_type](client, buf, buflen);
}

void ChatServer::dealLoginRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_LOGIN_RQ *rq = (STRU_LOGIN_RQ *)buf;
    std::list<std::string> lstRes;
    int uuid          = 0;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select uuid, password from t_user where username = '%s';", rq->username);
    if (!m_sql.SelectMySql(sqlbuf, 100, lstRes)) {
        SYLAR_LOG_ERROR(g_logger) << "select error:" << sqlbuf;
    }
    STRU_LOGIN_RS rs;
    if (lstRes.size() == 0) {
        rs.result = user_not_exist;
    } else {
        uuid = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        std::string password = lstRes.front().c_str();
        lstRes.pop_front();
        if (strcmp(password.c_str(), rq->password) != 0) {
            rs.result = password_error;
        } else {
            rs.result = login_wait;
            rs.userid = uuid;
            SYLAR_LOG_INFO(g_logger) << __func__ << " select uuid =" << uuid;

            // 登录成功
            client->send((char *)&rs, sizeof(rs));
            // 将uuid和对应的服务套接字对应
            m_mapIdToSock[uuid] = client;

            // 获取好友列表
            getFriendList(client, (char *)&uuid, sizeof(rs.userid));
            rs.result = login_success;
        }
    }
    client->send((char *)&rs, sizeof(rs));
    if (rs.result == login_success) {
        // 用户登录成功, 推送历史消息
        SYLAR_LOG_DEBUG(g_logger) << "用户成功登录, 推送历史消息";
        pushOfflineMsg(client, uuid);
    }
}
void ChatServer::dealRegisterRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_REGISTER_RQ *rq = (STRU_REGISTER_RQ *)buf;
    // 先判断该username是否已经存在
    std::list<std::string> lstRes;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select username from t_user where username = '%s';", rq->username);
    if (!m_sql.SelectMySql(sqlbuf, 10, lstRes)) {
        SYLAR_LOG_ERROR(g_logger) << "SelectMySQL Error:" << sqlbuf;
        return;
    }
    STRU_REGISTER_RS rs;
    if (lstRes.size() > 0) {
        rs.result = user_is_exist;
        SYLAR_LOG_ERROR(g_logger) << "Register Failed: user_is_exist";
    } else {
        rs.result = register_success;
        sprintf(sqlbuf, "INSERT INTO `t_user` (`username`, `password`, `tel`) VALUES ('%s', '%s', '%s');", rq->username, rq->password, rq->tel);
        if (!m_sql.UpdateMySql(sqlbuf)) {
            SYLAR_LOG_ERROR(g_logger) << "UpdateMySQL Error:" << sqlbuf;
        }
    }
    if (!client->send((char *)&rs, sizeof(rs))) {
        SYLAR_LOG_ERROR(g_logger) << "Send Failed";
    }
}

void ChatServer::getFriendList(sylar::Socket::ptr client, const char *buf, int buflen) {
    int *uuid = (int *)buf;
    STRU_FRIEND_INFO info;
    getFriendInfoFromSql(*uuid, &info); // 获取自己的信息
    SYLAR_LOG_INFO(g_logger) << __func__ << " send 数据长度:" << sizeof(info);
    client->send((char *)&info, sizeof(info));

    // 获取好友信息列表
    std::list<std::string> lstRes;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select friend_id from t_friendship where uuid = '%d';", *uuid);
    if (!m_sql.SelectMySql(sqlbuf, 1, lstRes)) {
        SYLAR_LOG_ERROR(g_logger) << "getFriendList() SELECT Error:" << sqlbuf;
    }
    while (lstRes.size() > 0) {
        int friend_id = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        STRU_FRIEND_INFO friendinfo;
        getFriendInfoFromSql(friend_id, &friendinfo);
        // std::cout << "getFriendList() friend_id: " << friendinfo.uuid << std::endl;
        client->send((char *)&friendinfo, sizeof(friendinfo));
        // 如果好友在线, 将自己上线的信息发送给好友
        if (m_mapIdToSock.find(friendinfo.uuid) != m_mapIdToSock.end()) {
            auto sockFriend = m_mapIdToSock[friendinfo.uuid];
            sockFriend->send((char *)&info, sizeof(info));
        }
    }
}

void ChatServer::dealChatRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_CHAT_RQ *rq = (STRU_CHAT_RQ *)buf;
    // 持久化聊天数据
    // 1. 持久到消息同步库
    std::stringstream ss;
    ss << "INSERT INTO `t_message_synchronization`(`messageType`, `sendTo`, `sendFrom`, `messageContent`) VALUES("
       << "'文本', " << rq->friendid << ", " << rq->userid << ", '" << rq->content << "');";
    if (!m_sql.UpdateMySql(ss.str().c_str())) {
        SYLAR_LOG_ERROR(g_logger) << "UpdateMySQL Error:" << ss.str();
    }
    ss.str("");
    ss.clear();
    // 2. 持久到消息存储库
    ss << "insert into `t_message_repository`(`messageType`, `senderUserId`, `receiverUserId`, `messageContent`) VALUES("
       << "'文本', " << rq->userid << ", " << rq->friendid << ", "
       << "'" << rq->content << "'"
       << ");";
    if (!m_sql.UpdateMySql(ss.str().c_str())) {
        SYLAR_LOG_ERROR(g_logger) << "UpdateMySQL Error:" << ss.str();
    }
    ss.str("");
    ss.clear();

    if (m_mapIdToSock.find(rq->friendid) == m_mapIdToSock.end()) { // 好友离线
        STRU_CHAT_RS rs;
        rs.userid   = rq->userid;
        rs.friendid = rq->friendid;
        rs.result   = user_offline;
        SYLAR_LOG_INFO(g_logger) << "dealChatRq() rs.userid" << rs.userid << " rs.friendid: " << rs.friendid;
        client->send((char *)&rs, sizeof(rs));
        return;
    }
    // 好友在线
    auto sockFirend = m_mapIdToSock[rq->friendid];
    sockFirend->send(buf, buflen);
}
void ChatServer::dealAddFriendRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_ADD_FRIEND_RQ *rq = (STRU_ADD_FRIEND_RQ *)buf;
    // 1. 如果对方在线, 发送添加好友请求
    if (m_mapIdToSock.find(rq->receiverId) != m_mapIdToSock.end()) { // 对方在线
        m_mapIdToSock[rq->receiverId]->send(buf, buflen);
        sendIcon(m_mapIdToSock[rq->receiverId], rq->senderId, STRU_GET_USERICON_RS::NEWFRIEND);
        return;
    }
    // 1. 持久到消息同步库
    std::stringstream ss;
    ss << "INSERT INTO `t_message_synchronization`(`messageType`, `sendTo`, `sendFrom`, `messageContent`) VALUES("
       << "'好友申请', " << rq->receiverId << ", " << rq->senderId << ", '" << rq->senderName << "');";
    if (!m_sql.UpdateMySql(ss.str().c_str())) {
        SYLAR_LOG_ERROR(g_logger) << "UpdateMySQL Error:" << ss.str();
    }
    ss.str("");
    ss.clear();
}
void ChatServer::dealAddFriendRs(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_ADD_FRIEND_RS *rs = (STRU_ADD_FRIEND_RS *)buf;
    // SYLAR_LOG_INFO(g_logger) << __func__ << " recv buf" << rs->friendid << rs->friendName << rs->userid << rs->result;
    if (rs->result == add_success) { // 在好友关系表中加入该关系
        std::stringstream ss;
        ss << "insert into t_friendship values(" << rs->senderId << ", " << rs->receiverId << ");";
        std::string sqlbuf = ss.str();
        if (!m_sql.UpdateMySql(sqlbuf.c_str())) {
            SYLAR_LOG_DEBUG(g_logger) << "dealAddFriendRs() INSERT ERROR: " << sqlbuf;
        }
        ss.str("");
        ss.clear();
        ss << "insert into t_friendship values(" << rs->receiverId << ", " << rs->senderId << ");";
        sqlbuf = ss.str();
        if (!m_sql.UpdateMySql(sqlbuf.c_str())) {
            SYLAR_LOG_DEBUG(g_logger) << "dealAddFriendRs() INSERT ERROR: " << sqlbuf;
        }
        SYLAR_LOG_INFO(g_logger) << __func__ << " add friendship success";
    }
    if (m_mapIdToSock.count(rs->receiverId) > 0) {
        auto sock = m_mapIdToSock[rs->receiverId];
        sock->send(buf, buflen);
    }
}

void ChatServer::DealGetUserInfoRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    // TEST
    SYLAR_LOG_INFO(g_logger) << __func__;
    STRU_GET_USERINFO_RQ *rq = (STRU_GET_USERINFO_RQ *)buf;
    std::list<std::string> lstRes;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select icon from t_user where username = '%s';", rq->userName);
    m_sql.SelectMySql(sqlbuf, 1, lstRes);
    if (lstRes.size() != 1)
        return;
    std::string friendIconUrl = lstRes.front();
    lstRes.pop_front();
    STRU_GET_USERINFO_RS rs;
    strcpy(rs.szFileName, friendIconUrl.c_str());
    client->send((char *)&rs, sizeof(rs));
    /* SYLAR_LOG_INFO(g_logger) << __func__;
    STRU_GET_USERINFO_RQ *rq = (STRU_GET_USERINFO_RQ *)buf;
    std::list<std::string> lstRes;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select uuid from t_user where username = '%s';", rq->userName);
    if (!m_sql.SelectMySql(sqlbuf, 1, lstRes)) {
        SYLAR_LOG_DEBUG(g_logger) << "dealAddFriendRq() SELECT ERROR:" << sqlbuf;
    }
    if (lstRes.size() > 0) { // 存在该用户, 返回用户名和头像
        int userId = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        STRU_GET_USERINFO_RS rs;
        rs.result    = getinfo_success;
        rs.userId    = userId;
        strcpy(rs.userName, rq->userName);
        client->send((char *)&rs, sizeof(rs));
        sendIcon(client, userId); // 发送头像
    } else { // 不存在这个人
        STRU_GET_USERINFO_RS rs;
        rs.result = no_this_user;
        client->send((char *)&rs, sizeof(rs));
    } */
}

void ChatServer::getFriendInfoFromSql(int uuid, STRU_FRIEND_INFO *info) {
    info->uuid = uuid;
    std::list<std::string> lstRes;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select username, feeling from t_user where uuid = '%d';", uuid);
    if (!m_sql.SelectMySql(sqlbuf, 2, lstRes)) {
        SYLAR_LOG_DEBUG(g_logger) << "getFriendInfoFromSql() Select Error:" << sqlbuf;
    }
    if (lstRes.size() == 2) {
        strcpy(info->username, lstRes.front().c_str());
        lstRes.pop_front();
        strcpy(info->feeling, lstRes.front().c_str());
        lstRes.pop_front();
    }
    if (m_mapIdToSock.find(uuid) != m_mapIdToSock.end()) {
        info->state = 1;
    } else {
        info->state = 0;
    }
    SYLAR_LOG_INFO(g_logger) << "getFriendInfoFromSql() info->uuid: " << info->uuid << " info->username: " << info->username;
}

void ChatServer::DealOfflineRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_OFFLINE *rq = (STRU_OFFLINE *)buf;
    // 1. 更新上次登录时间
    updateOfflineTime(rq->uuid);
    // 2. 给这个请求发送者的所有好友发送下线通知
    std::list<std::string> lstRes;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select friend_id from t_friendship where uuid = '%d'", rq->uuid);
    if (!m_sql.SelectMySql(sqlbuf, 1, lstRes)) {
        SYLAR_LOG_DEBUG(g_logger) << __func__ << "SELECT ERROR:" << sqlbuf;
    }
    while (lstRes.size() > 0) {
        int friendid = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        if (m_mapIdToSock.find(friendid) != m_mapIdToSock.end()) {
            SYLAR_LOG_INFO(g_logger) << "告诉好友" << friendid << "好友" << rq->uuid << "下线请求";
            auto sockFriend = m_mapIdToSock[friendid];
            sockFriend->send(buf, buflen);
        }
    }
    if (m_mapIdToSock.find(rq->uuid) != m_mapIdToSock.end()) {
        m_mapIdToSock.erase(rq->uuid);
    }
}

bool ChatServer::updateOfflineTime(int userid) {
    std::stringstream ss;
    ss << "UPDATE `t_user` SET `last_login_time` = NOW() WHERE `uuid` = "
       << userid << ";";
    if (!m_sql.UpdateMySql(ss.str().c_str())) {
        SYLAR_LOG_ERROR(g_logger) << "UpdateMysql Error:" << ss.str();
        return false;
    }
    ss.str("");
    ss.clear();
    return true;
}

bool ChatServer::pushOfflineMsg(sylar::Socket::ptr client, int userid) {
    // 1. 推送离线聊天信息
    std::stringstream ss;
    ss << "SELECT `sendTo`, `sendFrom`, `messageContent`,`createTime` FROM `t_message_synchronization` WHERE `sendTo` = "
       << userid << " AND `messageType` = '文本' AND `createTime` >= (SELECT `last_login_time` FROM `t_user` WHERE `uuid` = "
       << userid << ");";
    std::list<std::string> lstRes;
    if (!m_sql.SelectMySql(ss.str().c_str(), 4, lstRes)) {
        SYLAR_LOG_ERROR(g_logger) << "SelectMySql Error:" << ss.str();
        return false;
    }
    ss.str("");
    ss.clear();
    while (lstRes.size() > 0) {
        STRU_CHAT_RQ rq;
        rq.friendid = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        rq.userid = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        strcpy(rq.content, lstRes.front().c_str());
        lstRes.pop_front();
        strcpy(rq.createTime, lstRes.front().substr(11, 8).c_str());
        lstRes.pop_front();
        client->send((char *)&rq, sizeof(rq));
    }
    lstRes.clear(); // 清空list容器
    // 2. 推送离线好友申请信息(名称+头像)
    ss << "SELECT `sendFrom`, `messageContent` FROM `t_message_synchronization` WHERE `sendTo` = " << userid << " AND `messageType` = '好友申请' "
       << "AND `createTime` >= (SELECT `last_login_time` FROM `t_user` WHERE `uuid` = " << userid << ");";
    m_sql.SelectMySql(ss.str().c_str(), 1, lstRes);
    ss.str("");
    ss.clear();
    while(lstRes.size() > 0) {
        int senderId = atoi(lstRes.front().c_str());
        lstRes.pop_front();
        std::string senderName = lstRes.front();
        lstRes.pop_front();
        STRU_ADD_FRIEND_RQ rq;
        rq.receiverId = userid;
        rq.senderId   = senderId;
        strcpy(rq.senderName, senderName.c_str());
        client->send((char *)&rq, sizeof(rq));
        sendIcon(client, senderId, STRU_GET_USERICON_RS::NEWFRIEND);
    }
    return true;
}

void ChatServer::DealFileInfoRq(sylar::Socket::ptr client, const char *buf, int nLen) {
    /* std::cout << __func__ << std::endl;
    STRU_FILE_INFO_RQ *rq = (STRU_FILE_INFO_RQ *)buf;
    FileInfo *info        = new FileInfo;
    
    info->nFileSize       = rq->nFileSize;
    info->nPos            = 0;
    strcpy(info->szFileName, rq->szFileName);
    strcpy(info->szFileId, rq->szFileId);
    std::stringstream ss;
    ss << "./../images/tempFile/" << info->szFileName;
    // 数据库存URL
    info->pFile = fopen(ss.str().c_str(), "w+");
    if (info->pFile == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "打开文件失败";
        return;
    }
    SYLAR_LOG_ERROR(g_logger) << ss.str();
    if (m_mapFileIdToFileInfo.find(info->szFileId) == m_mapFileIdToFileInfo.end()) {
        m_mapFileIdToFileInfo[info->szFileId] = info;
    }
    STRU_FILE_INFO_RS rs;
    rs.friendid = rq->uuid;
    rs.uuid     = rq->friendid;
    rs.nResult  = _file_accept;
    strcpy(rs.szFileId, rq->szFileId);
    client->send((char *)&rs, sizeof(rs));
    return; */
}

void ChatServer::DealFileInfoRs(sylar::Socket::ptr client, const char *buf, int nLen) {
   /*  std::cout << __func__ << std::endl;
    STRU_FILE_INFO_RS *rs = (STRU_FILE_INFO_RS *)buf;
    if (m_mapIdToSock.find(rs->friendid) == m_mapIdToSock.end()) {
        // 好友不在线
        return;
    } else {
        // 好友在线
        std::cout << "好友在线, 直接转发文件信息回复包" << std::endl;
        auto sockFriend = m_mapIdToSock[rs->friendid];
        sockFriend->send(buf, nLen);
    } */
}

void ChatServer::DealFileBlockRq(sylar::Socket::ptr client, const char *buf, int nLen) {
    /* std::cout << __func__ << std::endl;
    STRU_FILE_BLOCK_RQ *rq = (STRU_FILE_BLOCK_RQ *)buf;
    if (m_mapFileIdToFileInfo.find(rq->szFileId) == m_mapFileIdToFileInfo.end()) {
        SYLAR_LOG_ERROR(g_logger) << "没有保存文件信息，无法保存该文件块";
        return;
    }
    FileInfo *info = m_mapFileIdToFileInfo[rq->szFileId];
    int nResult    = fwrite(rq->szFileContent, sizeof(char), rq->nBlockSize, info->pFile);
    info->nPos += nResult;
    if (info->nPos >= info->nFileSize) { // 已经收到所有文件块
        fclose(info->pFile);             // 关闭文件指针
        m_mapFileIdToFileInfo.erase(rq->szFileId);
        delete info;
        info = nullptr;
    }
    // 旧的处理流程
    if (m_mapIdToSock.find(rq->friendid) == m_mapIdToSock.end()) {
        // 好友不在线
        return;
    } else {
        // 好友在线
        auto sockFriend = m_mapIdToSock[rq->friendid];
        sockFriend->send(buf, nLen);
    } */
}

void ChatServer::DealFileBlockRs(sylar::Socket::ptr client, const char *buf, int nLen) {
    std::cout << __func__ << std::endl;
    STRU_FILE_BLOCK_RS *rs = (STRU_FILE_BLOCK_RS *)buf;
    if (m_mapIdToSock.find(rs->friendid) == m_mapIdToSock.end()) {
        // 好友不在线
        return;
    } else {
        // 好友在线
        auto sockFriend = m_mapIdToSock[rs->friendid];
        sockFriend->send(buf, nLen);
    }
}

std::string ChatServer::GetFileName(const char *path) {
    int nlen = strlen(path);
    if (nlen < 1) {
        return std::string();
    }
    for (int i = nlen - 1; i >= 0; i--) {
        if (path[i] == '\\' || path[i] == '/') {
            return &path[i + 1];
        }
    }
    return std::string();
}

void ChatServer::sendIcon(sylar::Socket::ptr client, int friendId, STRU_GET_USERICON_RS::Flag flag) {// 发送friendId头像信息给Client
    STRU_GET_USERICON_RS rs;
    rs.userid     = friendId;
    rs.flag = flag;
    uint64_t nPos = 0;
    int nReadLen  = 0;
    
    std::list<std::string> lstRes;
    char sqlbuf[1024] = "";
    sprintf(sqlbuf, "select icon from t_user where uuid = '%d';", friendId);
    m_sql.SelectMySql(sqlbuf, 1, lstRes);
    if (lstRes.size() != 1)
        return;
    std::string friendIconUrl = lstRes.front();
    lstRes.pop_front();
    FILE *fd = fopen(friendIconUrl.c_str(), "rb");
    if (fd < 0) {
        SYLAR_LOG_ERROR(g_logger) << "打开头像文件失败";
        fclose(fd);
        return;
    }
    while (true) {
        nReadLen = fread(rs.szFileContent, sizeof(char), _DEF_FILE_CONTENT_SIZE, fd);
        if (nReadLen == -1) {
            SYLAR_LOG_ERROR(g_logger) << "read failed";
            break;
        } else if (nReadLen == 0 && ferror(fd)) {
            SYLAR_LOG_INFO(g_logger) << "读取文件出错:" << ferror(fd);
            break;
        } else if (nReadLen == 0 && feof(fd)) {
            SYLAR_LOG_INFO(g_logger) << "文件读取完毕:" << feof(fd);
            break;
        }
        rs.nBlockSize = nReadLen;
        client->send((char *)&rs, sizeof(rs));
        nPos += nReadLen;
    }
    fclose(fd);
}

void ChatServer::dealFileContentRq(sylar::Socket::ptr client, const char* buf, int buflen) {
    // GET需要返回结果, POST无需返回结果
    /* STRU_FILE_CONTENT_RQ* rq = (STRU_FILE_CONTENT_RQ*)buf; */
    
    
}
void ChatServer::dealFileContentRs(sylar::Socket::ptr client, const char *buf, int buflen) {

}

void sendFile(sylar::Socket::ptr client, const std::string& filePath) {
    /* int fd = open(filePath.c_str(), O_RDONLY) // 以二进制方式打开文件
    if (fd == -1) {
        SYLAR_LOG_ERROR(g_logger) << __func__ << " ifstream打开文件失败";
        return;
    }
    STRU_FILE_CONTENT_RS rs; // 分块发送, 块大小8K
    sendfile(client->getSocket(), fd); */
}

} // namespace chat
