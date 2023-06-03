/*
 * @Author: Cui XiaoJun
 * @Date: 2023-05-13 14:23:10
 * @LastEditTime: 2023-05-13 14:26:10
 * @email: cxj2856801855@gmail.com
 * @github: https://github.com/SocialistYouth/
 * @details 通常在IM系统中，消息会有以下几类：文本消息、表情消息、图片消息、视频消息、文件消息等等
 */
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <string.h>
#include <stdint.h>


namespace chat {
#define _MAX_SIZE	(40)
#define _DEF_CONTENT_SIZE (1024)

// 协议头
#define _DEF_PROTOCOL_BASE (100)
#define _DEF_PROTOCOL_COUNT (100)

// 注册
#define _DEF_PACK_REGISTER_RQ	(_DEF_PROTOCOL_BASE + 0 )
#define _DEF_PACK_REGISTER_RS	(_DEF_PROTOCOL_BASE + 1 )
//登录
#define _DEF_PACK_LOGIN_RQ	(_DEF_PROTOCOL_BASE + 2 )
#define _DEF_PACK_LOGIN_RS	(_DEF_PROTOCOL_BASE + 3 )
//好友信息
#define _DEF_PACK_FRIEND_INFO	(_DEF_PROTOCOL_BASE + 4 )
//添加好友
#define _DEF_PACK_ADDFRIEND_RQ	(_DEF_PROTOCOL_BASE + 5 )
#define _DEF_PACK_ADDFRIEND_RS	(_DEF_PROTOCOL_BASE + 6 )
//聊天
#define _DEF_PACK_CHAT_RQ	(_DEF_PROTOCOL_BASE + 7 )
#define _DEF_PACK_CHAT_RS	(_DEF_PROTOCOL_BASE + 8 )
//离线
#define _DEF_PACK_OFFLINE_RQ	(_DEF_PROTOCOL_BASE + 9 )
/*文件传输*/ 
// 文件信息
#define _DEF_PROTOCOL_FILE_INFO_RQ (_DEF_PROTOCOL_BASE + 10)
#define _DEF_PROTOCOL_FILE_INFO_RS (_DEF_PROTOCOL_BASE + 11)
// 文件块
#define _DEF_PROTOCOL_FILE_BLOCK_RQ (_DEF_PROTOCOL_BASE + 12)
#define _DEF_PROTOCOL_FILE_BLOCK_RS (_DEF_PROTOCOL_BASE + 13)
// 想要添加的好友信息
#define _DEF_PROTOCOL_GETUSERINFO_RQ (_DEF_PROTOCOL_BASE + 14)
#define _DEF_PROTOCOL_GETUSERINFO_RS (_DEF_PROTOCOL_BASE + 15)
#define _DEF_PROTOCOL_GETUSERICON_RS (_DEF_PROTOCOL_BASE + 16)

// 最大文件路径长度
#define _MAX_FILE_PATH_SIZE (512)
// 最大文件大小
#ifndef _DEF_FILE_CONTENT_SIZE
#define _DEF_FILE_CONTENT_SIZE (8*1024)
#endif
//返回的结果
//注册请求的结果
#define user_is_exist		(0)
#define register_success	(1)
//登录请求的结果
#define user_not_exist		(0)
#define password_error		(1)
#define login_success		(2)
#define login_wait          (3)
//添加好友的结果 & 获取用户信息返回的结果
#define no_this_user		(0)
#define user_refuse			(1)
#define user_offline		(2)
#define add_success			(3)
#define getinfo_success     (4)


//协议结构


/**
 * @brief 注册请求块
*/
typedef struct STRU_REGISTER_RQ
{
    typedef int PackType;
    STRU_REGISTER_RQ() :type(_DEF_PACK_REGISTER_RQ)
    {
        memset(tel, 0, sizeof(tel));
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
    }
    //需要手机号码 , 密码, 昵称

    /**
     * @brief 数据包类型：_DEF_PACK_REGISTER_RQ
    */
    PackType type;
    /**
     * @brief 电话号码
    */
    char tel[_MAX_SIZE];
    /**
     * @brief 用户名
    */
    char username[_MAX_SIZE];
    /**
     * @brief 用户密码
    */
    char password[_MAX_SIZE];

}STRU_REGISTER_RQ;

/**
 * @brief 注册回复块
*/
typedef struct STRU_REGISTER_RS
{
    typedef int PackType;
    //回复结果
    STRU_REGISTER_RS() : type(_DEF_PACK_REGISTER_RS), result(register_success)
    {
    }
    /**
     * @brief 数据包类型：_DEF_PACK_REGISTER_RS
    */
    PackType type;
    /**
     * @brief 回复结果
    */
    int result;

}STRU_REGISTER_RS;


/**
 * @brief 登录请求块
*/
typedef struct STRU_LOGIN_RQ
{
    typedef int PackType;
    //登录需要: 手机号 密码
    STRU_LOGIN_RQ() :type(_DEF_PACK_LOGIN_RQ)
    {
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
    }
    /**
     * @brief 数据包类型：_DEF_PACK_LOGIN_RQ
    */
    PackType type;
    /**
     * @brief 用户名
    */
    char username[_MAX_SIZE];
    /**
     * @brief 用户密码
    */
    char password[_MAX_SIZE];

}STRU_LOGIN_RQ;

/**
 * @brief 登录回复块
*/
typedef struct STRU_LOGIN_RS
{
    typedef int PackType;
    // 需要 结果 , 用户的id
    STRU_LOGIN_RS() : type(_DEF_PACK_LOGIN_RS), result(password_error), userid(0)
    {
    }
    /**
     * @brief 数据包类型：_DEF_PACK_REGISTER_RS
    */
    PackType type;
    /**
     * @brief 回复结果
    */
    int result;
    /**
     * @brief 登录用户的uuid
    */
    int userid;

}STRU_LOGIN_RS;

/**
 * @brief 好友信息块
 * @details 包括自身
*/
typedef struct STRU_FRIEND_INFO
{
    typedef int PackType;
    STRU_FRIEND_INFO() :type(_DEF_PACK_FRIEND_INFO), uuid(0), iconid(0), state(0)
    {
        memset(username, 0, sizeof(username));
        memset(feeling, 0, sizeof(feeling));
    }
    //需要 用户id 头像id 昵称 签名 状态--是否在线
    /**
     * @brief 数据包类型：_DEF_PACK_FRIEND_INFO
    */
    PackType type;
    /**
     * @brief 用户唯一id
    */
    int uuid;
    /**
     * @brief 头像id
    */
    int iconid;
    /**
     * @brief 用户状态(在线|离线)
    */
    int state;
    /**
     * @brief 用户名
    */
    char username[_MAX_SIZE];
    /**
     * @brief 个性签名
    */
    char feeling[_MAX_SIZE];

}STRU_FRIEND_INFO;


/**
 * @brief 添加好友请求块
*/
typedef struct STRU_ADD_FRIEND_RQ
{
    typedef int PackType;
    // 如果用户1 添加用户2 为好友 需要 用户1 id 用户1 名字 ,用户2的名字
    STRU_ADD_FRIEND_RQ() :type(_DEF_PACK_ADDFRIEND_RQ), senderId(0), receiverId(0)
    {
        memset(senderName, 0, sizeof(senderName));
        memset(receiverName, 0, sizeof(receiverName));
    }
    /**
     * @brief 数据包类型: _DEF_PACK_ADDFRIEND_RQ
    */
    PackType type;
    /**
     * @brief 发送端id
    */
    int senderId;
    /**
     * @brief 发送端name
    */
    char senderName[_MAX_SIZE];
    /**
     * @brief 接收端id
     */
    int receiverId;
    /**
     * @brief 接收端name
     */
    char receiverName[_MAX_SIZE];

}STRU_ADD_FRIEND_RQ;

/**
 * @brief 添加好友回复块
*/
typedef struct STRU_ADD_FRIEND_RS
{
    typedef int PackType;
    STRU_ADD_FRIEND_RS() :type(_DEF_PACK_ADDFRIEND_RS), senderId(0), receiverId(0), result(add_success)
    {
        memset(senderName, 0, sizeof(senderName));
    }
    /// @brief 数据包类型: _DEF_PACK_ADDFRIEND_RS
    PackType type;
    /// @brief 发送端id
    int senderId; 
    /// @brief 接收端id
    int receiverId;
    /// @brief 回复结果
    int result;
    /// @brief 发送端Name
    char senderName[_MAX_SIZE];
}STRU_ADD_FRIEND_RS;

typedef struct STRU_GET_USERINFO_RQ{
    typedef int PackType;
    STRU_GET_USERINFO_RQ() : type(_DEF_PROTOCOL_GETUSERINFO_RQ), senderId(0)
    {
        memset(userName, 0, sizeof(userName));
        memset(senderName, 0, sizeof(senderName));
    }
    /**
     * @brief 数据包类型: _DEF_PROTOCOL_GETUSERINFO_RQ
    */
    PackType type;
    /**
     * @brief 用户Name
    */
    char userName[_MAX_SIZE];
    /**
     * @brief 发送者Id
    */
    int senderId;
    /**
     * @brief 发送者Name
    */
    char senderName[_MAX_SIZE];
}STRU_GET_USERINFO_RQ;

typedef struct STRU_GET_USERINFO_RS{
    typedef int PackType;
    STRU_GET_USERINFO_RS() : type(_DEF_PROTOCOL_GETUSERINFO_RS), userId(0), result(no_this_user), nFileSize(0)
    {
        memset(userName, 0, sizeof(userName));
        memset(szFileId, 0, _MAX_FILE_PATH_SIZE);
        memset(szFileName, 0, _MAX_FILE_PATH_SIZE);
    }
    /**
     * @brief 数据包类型: _DEF_PROTOCOL_GETUSERINFO_RS
    */
    PackType type;
    /// 用户id
    int userId;
    /// 用户名
    char userName[_MAX_SIZE];
    /**
     * @brief 回复结果
    */
    int result;
    /**
     * @brief 文件唯一id
    */
    char szFileId[_MAX_FILE_PATH_SIZE];
    /**
     * @brief 文件名
    */
    char szFileName[_MAX_FILE_PATH_SIZE];
    /**
     * @brief 文件大小
    */
    uint64_t nFileSize;
} STRU_GET_USERINFO_RS;

typedef struct STRU_GET_USERICON_RS{
    typedef int PackType;
    enum Flag {
        USERINFO, // 添加好友时查看的头像
        NEWFRIEND, // 接受好友声请时查看的头像
    };
    STRU_GET_USERICON_RS() : type(_DEF_PROTOCOL_GETUSERICON_RS), userid(0), flag(USERINFO)
    {
        memset(szFileId, 0, _MAX_FILE_PATH_SIZE);
        memset(szFileContent, 0, _DEF_FILE_CONTENT_SIZE);
    }
    /**
     * @brief 数据包类型: _DEF_PROTOCOL_GETUSERICON_RS
     */
    PackType type;
    /**
     * @brief 发送者的用户id
     */
    int userid;
    /**
     * @brief 该好友头像的用处
     */
    Flag flag;
    /**
     * @brief 文件唯一id
    */
    char szFileId[_MAX_FILE_PATH_SIZE];
    /**
     * @brief 文件块内容
    */
    char szFileContent[_DEF_FILE_CONTENT_SIZE];
    /**
     * @brief 文件块大小
    */
    uint64_t nBlockSize;
} STRU_GET_USERICON_RS;

/**
 * @brief 聊天内容请求块
*/
typedef struct STRU_CHAT_RQ
{
    typedef int PackType;
    STRU_CHAT_RQ() :type(_DEF_PACK_CHAT_RQ), userid(0), friendid(0)
    {
        memset(content, 0, _DEF_CONTENT_SIZE);
        memset(createTime, 0, 20);
    }
    // 谁发给谁 服务器转发  用户1 id 用户2 id 发的内容
    /**
     * @brief 数据包类型: _DEF_PACK_CHAT_RQ
    */
    PackType type;
    /**
     * @brief 用户唯一id
    */
    int userid;
    /**
     * @brief 好友id
    */
    int friendid;
    /**
     * @brief 聊天信息块的创建时间
     */
    char createTime[20];
    /**
     * @brief 聊天内容
     */
    char content[_DEF_CONTENT_SIZE];

}STRU_CHAT_RQ;

/**
 * @brief 聊天内容回复块
*/
typedef struct STRU_CHAT_RS
{
    typedef int PackType;
    STRU_CHAT_RS() :type(_DEF_PACK_CHAT_RS), userid(0), friendid(0), result(0) {}
    /**
     * @brief 数据包类型: _DEF_PACK_CHAT_RS
    */
    PackType type;
    /**
     * @brief 用户唯一id
    */
    int userid;
    /**
     * @brief 好友id
    */
    int friendid; //方便找是哪个人不在线
    /**
     * @brief 回复结果
    */
    int result;

}STRU_CHAT_RS;

/**
 * @brief 离线通知块
*/
typedef struct STRU_OFFLINE {
    typedef int PackType;
    STRU_OFFLINE() : type(_DEF_PACK_OFFLINE_RQ) {}
    /**
     * @brief 数据包类型: _DEF_PACK_OFFLINE_RQ
    */
    PackType type;
    /**
     * @brief 用户唯一id
    */
    int uuid;
}STRU_OFFLINE;


// fflush
// 传文件流程
// 秒传，分享(发)
// 断点续传
// 零拷贝
// 文件切片（滑动窗口）

// 文件传输协议

// 1. Client<->Server<->Client
/**
 * @brief 文件信息
*/
/// MD5字符数组大小
#define _MD5_STR_SIZE 33
struct FileInfo
{
    FileInfo() : nPos(0), nFileSize(0), pFile(nullptr) {
        memset(fileId  , 0, _MAX_FILE_PATH_SIZE);
        memset(fileName, 0, _MAX_FILE_PATH_SIZE);
        memset(filePath, 0, _MAX_FILE_PATH_SIZE);
        memset(md5     , 0, _MD5_STR_SIZE      );
    }
    /// @brief 文件唯一id
    char fileId[_MAX_FILE_PATH_SIZE];
    /// @brief 文件名
    char fileName[_MAX_FILE_PATH_SIZE];
    /// @brief 文件所在路径
    char filePath[_MAX_FILE_PATH_SIZE];
    /// @brief 文件MD5值
    char md5[_MD5_STR_SIZE];
    /// @brief 文件已经接受的字节数
    uint64_t nPos;
    /// @brief 文件大小
    uint64_t nFileSize;
    /// @brief 文件指针
    FILE* pFile;
};
/**
 * @brief 文件信息请求
*/
struct STRU_FILE_INFO_RQ {
    typedef int PackType;
    STRU_FILE_INFO_RQ() : type(_DEF_PROTOCOL_FILE_INFO_RQ), fileSize(0), senderId(0), receiverId(0) {
        memset(fileId  , 0, _MAX_FILE_PATH_SIZE);
        memset(fileName, 0, _MAX_FILE_PATH_SIZE);
        memset(md5, 0, _MD5_STR_SIZE);
    }
    /// @brief 数据包类型: _DEF_PROTOCOL_FILE_INFO_RQ
    PackType type;
    /// @brief 文件唯一id
    char fileId[_MAX_FILE_PATH_SIZE];
    /// @brief 文件名
    char fileName[_MAX_FILE_PATH_SIZE];
    /// @brief 文件MD5值
    char md5[_MD5_STR_SIZE];
    /// @brief 文件大小
    uint64_t fileSize;
    /// @brief 发送者id
    int senderId;
    /// @brief 接收者id
    int receiverId;
};

/**
 * @brief 文件信息回复
*/
struct STRU_FILE_INFO_RS 
{
    enum kResult {
        ACCEPT,
        REFUSE
    };
    typedef int PackType;
    STRU_FILE_INFO_RS() : type(_DEF_PROTOCOL_FILE_INFO_RS), result(ACCEPT), senderId(0), receiverId(0) {
        memset(szFileId, 0, _MAX_FILE_PATH_SIZE);
    }
    /// @brief 数据包类型: _DEF_PROTOCOL_FILE_INFO_RS
    PackType type;
    /// @brief 文件唯一id
    char szFileId[_MAX_FILE_PATH_SIZE];
    /// @brief 回复结果
    kResult result;
    /// @brief 用户唯一id
    int senderId;
    /// @brief 接收者Id
    int receiverId;
};

/**
 * @brief 文件块请求
*/
struct STRU_FILE_BLOCK_RQ
{
    typedef int PackType;
    STRU_FILE_BLOCK_RQ() : type(_DEF_PROTOCOL_FILE_BLOCK_RQ), blockSize(0), senderId(0), receiverId(0) {
        memset(fileId, 0, _MAX_FILE_PATH_SIZE);
        memset(fileContent, 0, _DEF_FILE_CONTENT_SIZE);
    }
    /// @brief 数据包类型: _DEF_PROTOCOL_FILE_BLOCK_RQ
    PackType type;
    /// @brief 文件唯一id
    char fileId[_MAX_FILE_PATH_SIZE];
    /// @brief 文件块内容
    char fileContent[_DEF_FILE_CONTENT_SIZE];
    /// @brief 文件块大小
    uint64_t blockSize;
    /// @brief 用户唯一id
    int senderId;
    /// @brief 接收者Id
    int receiverId;
};

#define _file_block_recv_success (0)
#define _file_block_recv_fail    (1)
/**
 * @brief 文件块接受回复
*/
struct STRU_FILE_BLOCK_RS
{
    typedef int PackType;
    STRU_FILE_BLOCK_RS() :type(_DEF_PROTOCOL_FILE_BLOCK_RS), nResult(_file_block_recv_success), uuid(0), friendid(0) {
        memset(fileId, 0, _MAX_FILE_PATH_SIZE);
    }
    /// @brief 数据包类型: _DEF_PROTOCOL_FILE_BLOCK_RS
    PackType type;
    /// @brief 文件唯一id
    char fileId[_MAX_FILE_PATH_SIZE];
    /// @brief 回复结果
    int nResult;
    /// @brief 用户唯一id
    int uuid;
    /// @brief 好友id
    int friendid;
};

/* // 1. Client<->Server
struct STRU_FILE_CONTENT_RQ {
    typedef int PackType;
    enum Method {
        GET,
        POST
    };
    STRU_FILE_CONTENT_RQ() {
    }
    /// @brief 
    PackType type;
    /// @brief
    Method method;
    /// @brief 
    char filePath[_MAX_FILE_PATH_SIZE];
    
};

struct STRU_FILE_CONTENT_RS {
    typedef int PackType;
    /// @brief 数据包类型: _DEF_PROTOCOL_FILE_BLOCK_RQ
    PackType type;
    /// @brief 文件唯一id
    char fileId[_MAX_FILE_PATH_SIZE];
    /// @brief 文件块内容
    char fileContent[_DEF_FILE_CONTENT_SIZE];
    /// @brief 文件块大小
    uint64_t blockSize;
}; */
} // namespace chat

#endif //__PROTOCOL_H__
