/*
 * @Author: Cui XiaoJun
 * @Date: 2023-05-13 14:08:41
 * @LastEditTime: 2023-05-13 14:55:33
 * @email: cxj2856801855@gmail.com
 * @github: https://github.com/SocialistYouth/
 */
#include "chatserver.h"

namespace chat {
sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");
ChatServer::ChatServer(sylar::IOManager *worker /* = sylar::IOManager::GetThis() */,
                       sylar::IOManager *accept_worker /* = sylar::IOManager::GetThis() */)
    : TcpServer(worker, accept_worker) {
    setProtocolMap();
}
void ChatServer::setProtocolMap() {
#define XX(str, func)                                                                                                    \
    {                                                                                                                    \
        auto call = std::bind(&ChatServer::func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); \
        m_deal_items.insert({str, call});                                                                                \
    }

    XX(_DEF_PACK_LOGIN_RQ, dealLoginRq);
    XX(_DEF_PACK_REGISTER_RQ, dealRegisterRq);
    XX(_DEF_PACK_CHAT_RQ, dealChatRq);
    XX(_DEF_PACK_ADDFRIEND_RQ, dealAddFriendRq);
    XX(_DEF_PACK_ADDFRIEND_RS, dealAddFriendRs);

/*     XX(_DEF_PROTOCOL_FILE_INFO_RQ, DealFileInfoRq);
    XX(_DEF_PROTOCOL_FILE_INFO_RS, DealFileInfoRs);
    XX(_DEF_PROTOCOL_FILE_BLOCK_RQ, DealFileBlockRq);
    XX(_DEF_PROTOCOL_FILE_BLOCK_RS, DealFileBlockRs); */
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
	int nPackSize = 0;	// 存储包大小
    int rt        = 0;  // 一次接受到的字节数
    while (true) {
        rt = client->recv((char*)&nPackSize, sizeof(int));
        SYLAR_LOG_INFO(g_logger) << "收到包大小:" << nPackSize;
        if (rt <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "client error rt=" << rt
                                     << " errno=" << errno << " errstr=" << strerror(errno);
			break;
        }
        int offset = 0; // 从buf开始偏移多少
        char *recvbuf = new char[nPackSize];
        while (nPackSize) {
            if ((rt         = client->recv(recvbuf + offset, sizeof(recvbuf))) > 0) {
                SYLAR_LOG_INFO(g_logger) << "接受到一次包大小:" << rt;
                nPackSize -= rt;
                offset += rt;
			}
        }
        recvbuf[offset] = '\0';
        SYLAR_LOG_INFO(g_logger) << "recv: " << recvbuf;
        dealData(client, recvbuf, offset);
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
    int header_type = *(int*)buf;
    SYLAR_LOG_INFO(g_logger) << "DealData() header_type:" << header_type;
    if (header_type >= _DEF_PROTOCOL_BASE && header_type <= _DEF_PROTOCOL_BASE + _DEF_PROTOCOL_COUNT)
        m_deal_items[header_type](client, buf, buflen);
}


void ChatServer::dealLoginRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_LOGIN_RQ* rq = (STRU_LOGIN_RQ*)buf;
	std::list<std::string> lstRes;
	char sqlbuf[1024] = "";
	sprintf(sqlbuf, "select uuid, password from t_user where username = '%s';", rq->username);
	if (!m_sql.SelectMySql(sqlbuf, 100, lstRes)) {
        SYLAR_LOG_ERROR(g_logger) << "select error:" << sqlbuf;
    }
	STRU_LOGIN_RS rs;
	if (lstRes.size() == 0) {
		rs.result = user_not_exist;
	}
	else {
		int uuid = atoi(lstRes.front().c_str());
		lstRes.pop_front();
		std::string password = lstRes.front().c_str();
		lstRes.pop_front();
		if (strcmp(password.c_str(), rq->password) != 0) {
			rs.result = password_error;
		}
		else {
			rs.result = login_wait;
			rs.userid = uuid;
            SYLAR_LOG_INFO(g_logger) << __func__ << " select uuid =" << uuid;

            // 登录成功
            client->send((char *)&rs, sizeof(rs));
            // 将uuid和对应的服务套接字对应
			m_mapIdToSock[uuid] = client;

			// 获取好友列表
            sleep(1);
            getFriendList(client, (char*)&uuid, sizeof(rs.userid));
            rs.result = login_success;
        }
	}
	client->send((char *)&rs, sizeof(rs));
}
void ChatServer::dealRegisterRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_REGISTER_RQ* rq = (STRU_REGISTER_RQ*)buf;
	// 先判断该username是否已经存在
	std::list<std::string> lstRes;
	char sqlbuf[1024] = "";
	sprintf(sqlbuf, "select username from t_user where username = '%s'", rq->username);
	if (!m_sql.SelectMySql(sqlbuf, 10, lstRes)) {
		SYLAR_LOG_ERROR(g_logger) << "SelectMySQL Error:" << sqlbuf;
		return;
	}
	STRU_REGISTER_RS rs;
	if (lstRes.size() > 0) {
		rs.result = user_is_exist;
		SYLAR_LOG_ERROR(g_logger) << "Register Failed: user_is_exist";
	}
	else {
		rs.result = register_success;
		sprintf(sqlbuf, "INSERT INTO `t_user` (`username`, `password`, `tel`) VALUES ('%s', '%s', '%s')", rq->username, rq->password, rq->tel);
		if (!m_sql.UpdateMySql(sqlbuf)) {
			SYLAR_LOG_ERROR(g_logger) << "UpdateMySQL Error:" << sqlbuf;
		}
	}
	if (!client->send((char*)&rs, sizeof(rs))) {
		SYLAR_LOG_ERROR(g_logger) << "Send Failed";
	}
}

void ChatServer::getFriendList(sylar::Socket::ptr client, const char *buf, int buflen) {
    int* uuid = (int*)buf;
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
		client->send((char*)&friendinfo, sizeof(friendinfo));
		// 如果好友在线, 将自己上线的信息发送给好友
		if (m_mapIdToSock.find(friendinfo.uuid) != m_mapIdToSock.end()) {
			auto sockFriend = m_mapIdToSock[friendinfo.uuid];
			sockFriend->send((char*)&info, sizeof(info));
		}
	}
}

void ChatServer::dealChatRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_CHAT_RQ* rq = (STRU_CHAT_RQ*)buf;
	if (m_mapIdToSock.find(rq->friendid) == m_mapIdToSock.end()) { // 好友离线
		STRU_CHAT_RS rs;
		rs.userid = rq->userid;
		rs.friendid = rq->friendid;
		rs.result = user_offline;
		SYLAR_LOG_INFO(g_logger) << "dealChatRq() rs.userid" << rs.userid << " rs.friendid: " << rs.friendid;
		client->send((char*)&rs, sizeof(rs));
		return;
	}
	// 好友在线
	auto sockFirend = m_mapIdToSock[rq->friendid];
	sockFirend->send(buf, buflen);
}
void ChatServer::dealAddFriendRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    SYLAR_LOG_INFO(g_logger) << __func__ << "buf " << buf;
    STRU_ADD_FRIEND_RQ *rq = (STRU_ADD_FRIEND_RQ *)buf;
    // 查询数据库看friendName是否存在
	std::list<std::string> lstRes;
	char sqlbuf[1024] = "";
	sprintf(sqlbuf, "select uuid from t_user where username = '%s';", rq->friendName);
	if (!m_sql.SelectMySql(sqlbuf, 1, lstRes)) {
		SYLAR_LOG_DEBUG(g_logger) << "dealAddFriendRq() SELECT ERROR:" << sqlbuf;
	}
	if (lstRes.size() > 0) {
		int friendId = atoi(lstRes.front().c_str());
		lstRes.pop_front();
		// 查看该好友是否在线
		if (m_mapIdToSock.find(friendId) != m_mapIdToSock.end()) {
			// 在线
			// 向friend发送好友申请添加请求
			auto sockFriend = m_mapIdToSock[friendId];
			sockFriend->send(buf, buflen); // 直接转发
		}
		else {
			// 该用户不在线
			STRU_ADD_FRIEND_RS rs;
			rs.result = user_offline;
			rs.friendid = friendId;
			strcpy(rs.friendName, rq->friendName);
			rs.userid = rq->userid;
			client->send((char*)&rs, sizeof(rs));
		}
	}
	else {
		// 不存在这个人
		STRU_ADD_FRIEND_RS rs;
		rs.result = no_this_user;
		rs.friendid = 0;
		strcpy(rs.friendName, rq->friendName);
		rs.userid = rq->userid;
		client->send((char*)&rs, sizeof(rs));
	}
}
void ChatServer::dealAddFriendRs(sylar::Socket::ptr client, const char *buf, int buflen) {
	STRU_ADD_FRIEND_RS* rs = (STRU_ADD_FRIEND_RS*)buf;
	SYLAR_LOG_INFO(g_logger) << __func__ <<  " recv buf" << rs->friendid << rs->friendName << rs->userid << rs->result;
	STRU_ADD_FRIEND_RS rq;
	if (rs->result == add_success) { // 在好友关系表中加入该关系
		char sqlbuf[1024] = "";
		sprintf(sqlbuf, "insert into t_friendship values(%d, %d);", rs->friendid, rs->userid);
		if (!m_sql.UpdateMySql(sqlbuf)) {
			SYLAR_LOG_DEBUG(g_logger) << "dealAddFriendRs() INSERT ERROR: " << sqlbuf;
		}
        memset(sqlbuf, 0, sizeof(sqlbuf));
        sprintf(sqlbuf, "insert into t_friendship values(%d, %d);", rs->userid, rs->friendid);
        if (!m_sql.UpdateMySql(sqlbuf)) {
			SYLAR_LOG_DEBUG(g_logger) << "dealAddFriendRs() INSERT ERROR: " << sqlbuf;
		}

		getFriendList(client, (char*)&rs->friendid, buflen);
	}
	if (m_mapIdToSock.count(rs->userid) > 0) {
		auto sock = m_mapIdToSock[rs->userid];
		sock->send(buf, buflen);
	}
}
void ChatServer::getFriendInfoFromSql(int uuid, STRU_FRIEND_INFO* info) {
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
	}
	else {
		info->state = 0;
	}
	SYLAR_LOG_INFO(g_logger) << "getFriendInfoFromSql() info->uuid: " << info->uuid << " info->username: " << info->username;
}

void ChatServer::DealOfflineRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_OFFLINE* rq = (STRU_OFFLINE*)buf;
	// 给这个请求发送者的所有好友发送下线通知
	std::list<std::string> lstRes;
	char sqlbuf[1024] = "";
	sprintf(sqlbuf, "select friend_id from t_friendship where uuid = '%d'", rq->uuid);
	if (!m_sql.SelectMySql(sqlbuf, 1, lstRes)) {
		SYLAR_LOG_DEBUG(g_logger) << __func__ << "SELECT ERROR:" << sqlbuf;
	}
	while (lstRes.size() > 0) {
		int friendid = atoi(lstRes.front().c_str());
        SYLAR_LOG_INFO(g_logger) << "告诉好友" << friendid << "好友" << rq->uuid << "下线请求";
        lstRes.pop_front();
        if (m_mapIdToSock.find(friendid) != m_mapIdToSock.end()) {
			auto sockFriend = m_mapIdToSock[friendid];
			sockFriend->send(buf, buflen);
		}
	}
	if (m_mapIdToSock.find(rq->uuid) != m_mapIdToSock.end()) {
		m_mapIdToSock.erase(rq->uuid);
	}
}

} // namespace chat