/*
 * @Author: Cui XiaoJun
 * @Date: 2023-05-13 13:46:39
 * @LastEditTime: 2023-05-13 15:29:52
 * @email: cxj2856801855@gmail.com
 * @github: https://github.com/SocialistYouth/
 */
#ifndef __CHATSERVER_H__
#define __CHATSERVER_H__

#include <memory>

#include "MySQLConnection.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/macro.h"
#include "sylar/tcp_server.h"
#include "sylar/util.h"
#include "protocol.h"

namespace chat {
/**
 * @brief IM服务器
 */
class ChatServer : public sylar::TcpServer {
public:
    typedef std::shared_ptr<ChatServer> ptr;
    ChatServer(sylar::IOManager *worker        = sylar::IOManager::GetThis(),
               sylar::IOManager *accept_worker = sylar::IOManager::GetThis());
    virtual ~ChatServer() {}
    /* IM服务器应该具有的功能 */
    // 该函数用于处理客户端连接，接收客户端的请求，并根据请求类型调用不同的处理函数。
    void handleClient(sylar::Socket::ptr client) override;
    /**
	 * @brief 设置函数映射
	 */
	void setProtocolMap();
    /**
     * @brief 事件分发器
     * @param[in] client 处理与客户端连接的socket
     * @param[in] buf
     * @param[in] buflen
     */
    void dealData(sylar::Socket::ptr client, const char *buf, int buflen);
    /* 该函数用于处理客户端发送的登录请求，验证用户名和密码是否正确 */
    void dealLoginRq(sylar::Socket::ptr client, const char *buf, int buflen);
    /* 该函数用于处理客户端发送的登出请求，从在线用户列表中删除该用户，并向其他在线用户发送该用户下线的通知*/
    void dealRegisterRq(sylar::Socket::ptr client, const char *buf, int buflen);
    /**
	 * @brief 获得好友列表
	 * @param lSendIP
	 * @param buf
	 * @param nLen
	*/
	void getFriendList(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 处理聊天信息
	 * @param lSendIP
	 * @param buf
	 * @param nLen
	*/
	void dealChatRq(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 处理添加好友请求
	 * @param lSendIP 
	 * @param buf 
	 * @param nLen 
	*/
	void dealAddFriendRq(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 对方(好友)回复申请
	 * @param lSendIP 
	 * @param buf 
	 * @param nLen 
	*/
	void dealAddFriendRs(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 从数据库中查询用户信息
	 * @param uuid 用户id
	 * @param info 输出参数, 信息结构体
	*/
	void getFriendInfoFromSql(int uuid, STRU_FRIEND_INFO* info);

    /**
	 * @brief 处理下线请求
	 * @param lSendIP 
	 * @param buf 
	 * @param nLen 
	*/
	void DealOfflineRq(sylar::Socket::ptr client, const char *buf, int buflen);
    // 该函数用于初始化服务器，在该函数中可以注册路由和事件处理函数，以及启动定时任务等
    void init();

private:
    mysql::CMySql m_sql;
    /**
	 * @brief 协议头与处理函数的映射
	*/
    std::map<int, std::function<void(sylar::Socket::ptr, const char*, int)> > m_deal_items;
    /**
	 * @brief 用户id与套接字的映射
	*/
	std::map<int, sylar::Socket::ptr> m_mapIdToSock;
};

} // namespace chat

#endif //__CHATSERVER_H__