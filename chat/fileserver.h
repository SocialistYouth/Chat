
#ifndef __FILESERVER_H__
#define __FILESERVER_H__

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fileTransferProtocol.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/macro.h"
#include "sylar/tcp_server.h"
#include "sylar/util.h"

namespace file {
class FileServer : public sylar::TcpServer {
public:
    typedef std::shared_ptr<FileServer> ptr;
    FileServer(sylar::IOManager *worker        = sylar::IOManager::GetThis(),
               sylar::IOManager *accept_worker = sylar::IOManager::GetThis());
    virtual ~FileServer() {}

public:
    void handleClient(sylar::Socket::ptr client) override; // 该函数用于处理客户端连接，接收客户端的请求，并根据请求类型调用不同的处理函数。
    void init();                                           // 该函数用于初始化服务器，在该函数中可以注册路由和事件处理函数，以及启动定时任务等
    
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

public:
    /**
     * @brief 处理文件内容请求(GET/POST)
     * @param[] client 
     * @param[] buf 
     * @param[] buflen 
     */
    void dealFileContentRq(sylar::Socket::ptr client, const char *buf, int buflen);
    void dealFileBlockRq(sylar::Socket::ptr client, const char *buf, int buflen);

public:
    bool sendFile(sylar::Socket::ptr client, const std::string &filePath);
    // void saveFile();
    // void deleteFile();

public:
    /* ------------- 工具函数 -------------*/
    /**
     * @brief 根据路径得出文件名
     * @param[] path 文件路径
     * @return
     */
    static std::string GetFileName(const char *path);

private:
    /// @brief 协议头与处理函数的映射
    std::map<int, std::function<void(sylar::Socket::ptr, const char *, int)>> m_deal_items;
    /// @brief 文件Id与文件信息
    std::map<std::string, file::FileInfo *> m_mapFileIdToFileInfo;
};
} // namespace file

#endif //__FILESERVER_H__