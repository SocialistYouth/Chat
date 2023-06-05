#ifndef __CHATSERVER_H__
#define __CHATSERVER_H__

#include <memory>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include <sys/sendfile.h>

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
     */
    void dealData(sylar::Socket::ptr client, const char *buf, int buflen);
    /* 该函数用于处理客户端发送的登录请求，验证用户名和密码是否正确 */
    void dealLoginRq(sylar::Socket::ptr client, const char *buf, int buflen);
    /* 该函数用于处理客户端发送的登出请求，从在线用户列表中删除该用户，并向其他在线用户发送该用户下线的通知*/
    void dealRegisterRq(sylar::Socket::ptr client, const char *buf, int buflen);
    /**
	 * @brief 获得好友列表
	*/
	void getFriendList(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 处理聊天信息
	*/
	void dealChatRq(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 处理添加好友请求
	*/
	void dealAddFriendRq(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 对方(好友)回复申请
	*/
	void dealAddFriendRs(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 从数据库中查询用户信息
	 * @param uuid 用户id
	 * @param info 输出参数, 信息结构体
	*/
	void getFriendInfoFromSql(int uuid, STRU_FRIEND_INFO* info);
	/**
	 * @brief 处理文件传输信息请求
	*/
	void dealFileInfoRq(sylar::Socket::ptr client, const char* buf, int nLen);
	/**
	 * @brief 处理文件传输信息回复
	*/
	void DealFileInfoRs(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 处理文件块请求
	*/
	void DealFileBlockRq(sylar::Socket::ptr client, const char *buf, int buflen);
	/**
	 * @brief 处理文件块回复
	*/
	void DealFileBlockRs(sylar::Socket::ptr client, const char *buf, int buflen);
    void dealFileContentRq(sylar::Socket::ptr client, const char* buf, int buflen);
    void dealFileContentRs(sylar::Socket::ptr client, const char *buf, int buflen);
    /**
     * @brief 处理下线请求
     */
    void DealOfflineRq(sylar::Socket::ptr client, const char *buf, int buflen);
	
	void dealGetUserInfoRq(sylar::Socket::ptr client, const char* buf, int buflen);
	/**
	 * @brief 处理用户修改头像
	 */
    void dealAvatarUpdateRq(sylar::Socket::ptr client, const char* buf, int buflen);

    void dealAvatarUploadComplete(sylar::Socket::ptr client, const char *buf, int buflen);

    void init(); // 该函数用于初始化服务器，在该函数中可以注册路由和事件处理函数，以及启动定时任务等
public: // 功能函数
	/**
	 * @brief 更新当前用户的离线时间
	 * @param[in] userid 用户id
	 * @return  更新成功返回true, 失败返回false
	 */
	bool updateOfflineTime(int userid);
	/**
	 * @brief 推送离线期间的信息
	 * @param[] userid 用户id
	 * @return  推送成功返回true, 失败返回false
	 */
    bool pushOfflineMsg(sylar::Socket::ptr client, int userid);
    /**
     * @brief 根据路径得出文件名
     * @param[] path 文件路径
     * @return  
     */
	std::string GetFileName(const char *path);

	/**
	 * @brief 发送头像给client
	 * @param[] client 接受client
	 * @param[] friendId 发送id为friendid的头像
	 */
    void sendIcon(sylar::Socket::ptr client, int friendId, STRU_GET_USERICON_RS::Flag flag = STRU_GET_USERICON_RS::USERINFO);

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
	/**
	 * @brief 文件id与文件信息映射表
	 */
    std::map<std::string, FileInfo*> m_mapFileIdToFileInfo;
};

} // namespace chat

#endif //__CHATSERVER_H__ 