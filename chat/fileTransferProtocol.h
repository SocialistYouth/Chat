#ifndef __FILETRANSFERPROTOCOL_H__
#define __FILETRANSFERPROTOCOL_H__

#include <string.h>
#include <stdint.h>
#include <cstdio>
namespace file
{

enum PackType : uint32_t {
    FILE_CONTENT_RQ = 1000,
    FILE_CONTENT_RS
};

#ifndef _MAX_FILE_PATH_SIZE
#define _MAX_FILE_PATH_SIZE     (512)
#endif

#ifndef _DEF_FILE_CONTENT_SIZE
#define _DEF_FILE_CONTENT_SIZE  (8192)
#endif
// 文件传输协议

// 1. Client<->Server<->Client
/**
 * @brief 文件信息
*/
/// MD5字符数组大小
#ifndef _MD5_STR_SIZE
#define _MD5_STR_SIZE 33
#endif

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
/* struct STRU_FILE_INFO_RQ {
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
}; */

/**
 * @brief 文件信息回复
*/
/* struct STRU_FILE_INFO_RS 
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
}; */

/**
 * @brief 文件块请求
*/
/* struct STRU_FILE_BLOCK_RQ
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
}; */

/* #define _file_block_recv_success (0)
#define _file_block_recv_fail    (1) */
/**
 * @brief 文件块接受回复
*/
/* struct STRU_FILE_BLOCK_RS
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
}; */

// 1. Client<->Server
struct STRU_FILE_CONTENT_RQ {
    enum Method {
        GET,
        POST
    };
    STRU_FILE_CONTENT_RQ() : type(FILE_CONTENT_RQ) {
    }
    /// @brief 
    PackType type;
    /// @brief
    Method method;
    /// @brief 
    char filePath[_MAX_FILE_PATH_SIZE];
    
};

struct STRU_FILE_CONTENT_RS {
    /// @brief 数据包类型: _DEF_PROTOCOL_FILE_BLOCK_RQ
    PackType type;
    /// @brief 文件唯一id
    char fileId[_MAX_FILE_PATH_SIZE];
    /// @brief 文件块内容
    char fileContent[_DEF_FILE_CONTENT_SIZE];
    /// @brief 文件块大小
    uint64_t blockSize;
};

} // namespace file

#endif //__FILETRANSFERPROTOCOL_H__