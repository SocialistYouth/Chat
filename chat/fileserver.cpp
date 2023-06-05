#include "fileserver.h"

namespace file {
sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");
FileServer::FileServer(sylar::IOManager *worker /* = sylar::IOManager::GetThis() */,
                       sylar::IOManager *accept_worker /* = sylar::IOManager::GetThis() */)
    : TcpServer(worker, accept_worker) {
    setProtocolMap();
}

void FileServer::init() {
    auto addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8088");
    SYLAR_ASSERT(addr);
    std::vector<sylar::Address::ptr> addrs;
    addrs.push_back(addr);
    std::vector<sylar::Address::ptr> fails;
    while (!this->bind(addrs, fails)) {
        sleep(2);
    }
    this->start();
    SYLAR_LOG_INFO(g_logger) << "bind success, " << this->toString();
}

void FileServer::setProtocolMap() {
#define XX(str, func)                                                                                                        \
    {                                                                                                                        \
        auto call = std::bind(&FileServer::func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); \
        m_deal_items.insert({str, call});                                                                                    \
    }
    XX(FILE_CONTENT_RQ, dealFileContentRq);
    XX(FILE_BLOCK_RQ, dealFileBlockRq);
#undef XX
}

void FileServer::handleClient(sylar::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "new client: " << client->toString();
    int nPackSize = 0; // 存储包大小
    int rt        = 0; // 一次接受到的字节数
    while (true) {
        rt = client->recv((char *)&nPackSize, sizeof(int));
        SYLAR_LOG_INFO(g_logger) << "收到包大小:" << nPackSize;
        if (rt <= 0 && nPackSize > 100000) {
            SYLAR_LOG_ERROR(g_logger) << "恶意client error rt=" << rt
                                      << " nPackSize:" << nPackSize << " errno=" << errno << " errstr=" << strerror(errno);
            client->close();
            break;
        }
        int offset = 0; // 从buf开始偏移多少
        std::string recvbuf;
        recvbuf.resize(nPackSize);
        // char *recvbuf = new char[nPackSize];
        while (nPackSize) {
            if ((rt = client->recv(&recvbuf[0] + offset, nPackSize)) > 0) {
                nPackSize -= rt;
                offset += rt;
            }
        }
        recvbuf[offset] = '\0';
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

void FileServer::dealData(sylar::Socket::ptr client, const char *buf, int buflen) {
    unsigned int header_type = *(int *)buf;
    std::string type = "";
    switch (header_type) {
    case FILE_CONTENT_RQ:
        type = "FILE_CONTENT_RQ";
        break;
    case FILE_CONTENT_RS:
        type = "FILE_CONTENT_RQ";
        break;
    case FILE_BLOCK_RQ:
        type = "FILE_BLOCK_RQ";
        break;
    default:
        type = "UNKNOW";
        break;
    }
    SYLAR_LOG_INFO(g_logger) << "DealData() header_type:" << type;
    if (header_type >= FILE_CONTENT_RQ && header_type <= 1100)
        m_deal_items[header_type](client, buf, buflen);
}


void FileServer::dealFileContentRq(sylar::Socket::ptr client, const char* buf, int buflen) {
    SYLAR_LOG_DEBUG(g_logger) << __func__;
    STRU_FILE_CONTENT_RQ* rq = (STRU_FILE_CONTENT_RQ*)buf;
    if (rq->method == STRU_FILE_CONTENT_RQ::GET) {
        sendFile(client, _DEF_FILE_POS_PREFIX + std::string(rq->filePath));
    } else if (rq->method == STRU_FILE_CONTENT_RQ::POST) {
        file::FileInfo *fileInfo = new file::FileInfo;
        fileInfo->fileSize       = rq->fileSize;
        fileInfo->nPos           = 0;
        strcpy(fileInfo->fileId, rq->fileId);
        fileInfo->pFile = fopen((_DEF_FILE_POS_PREFIX + std::string(rq->filePath)).c_str(), "w+");
        if (fileInfo->pFile == nullptr) {
            SYLAR_LOG_ERROR(g_logger) << "打开文件失败: " << _DEF_FILE_POS_PREFIX + std::string(rq->filePath);
            return;
        }
        if (m_mapFileIdToFileInfo.find(fileInfo->fileId) == m_mapFileIdToFileInfo.end()) {
            m_mapFileIdToFileInfo[fileInfo->fileId] = fileInfo;
            SYLAR_LOG_DEBUG(g_logger) << __func__ << " 打开文件成功:" << _DEF_FILE_POS_PREFIX + std::string(rq->filePath);
        }
    }
}

void FileServer::dealFileBlockRq(sylar::Socket::ptr client, const char *buf, int buflen) {
    STRU_FILE_BLOCK_RQ* rq = (STRU_FILE_BLOCK_RQ*)buf;
    if (m_mapFileIdToFileInfo.find(rq->fileId) == m_mapFileIdToFileInfo.end()) {
        SYLAR_LOG_ERROR(g_logger) << __func__ << "发送的文件没有文件信息, 无法保存";
        return;
    }
    file::FileInfo *temp = m_mapFileIdToFileInfo[rq->fileId];
    auto fd              = temp->pFile;
    int nResult    = fwrite(rq->fileContent, sizeof(char), rq->blockSize, fd);
    temp->nPos += nResult;
    if (temp->nPos >= temp->fileSize) { // 已经收到所有文件块
        SYLAR_LOG_DEBUG(g_logger) << __func__ << "已经全部接受" << temp->fileName;
        fclose(fd);             // 关闭文件指针
        m_mapFileIdToFileInfo.erase(rq->fileId);
        delete temp;
        temp = nullptr;
    }
}

bool FileServer::sendFile(sylar::Socket::ptr client, const std::string &filePath) {
    SYLAR_LOG_DEBUG(g_logger) << __func__ << " filePath:" << filePath;
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd == -1) {
        SYLAR_LOG_ERROR(g_logger) << __func__ << "Failed to open file:" << GetFileName(filePath.c_str());
        return false;
    }
    // 使用 sendfile 进行零拷贝传输
    off_t offset = 0;
    ssize_t fileSize     = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    ssize_t sendSize = sendfile(client->getSocket(), fd, &offset, fileSize);
    SYLAR_LOG_INFO(g_logger) << "sendfile 发送文件大小:" << sendSize;
    return true;
}

std::string FileServer::GetFileName(const char *path) {
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

} // namespace file
