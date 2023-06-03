#include "chat/fileTransferProtocol.h"
#include "chat/protocol.h"
#include "chat/chatserver.h"
#include "chat/fileserver.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/macro.h"
#include "sylar/socket.h"
#include <stdlib.h>
#include <string>

#include <iostream>
#include <fcntl.h>
#include <unistd.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

std::string chatServer = "0.0.0.0:8080";
std::string fileServer = "0.0.0.0:8088";


void dealGetInfoRs(const char* recvbuf, int recvlen) {
    chat::STRU_GET_USERINFO_RS *rs = (chat::STRU_GET_USERINFO_RS *)recvbuf;
    auto path                      = rs->szFileName;
    bool ret;
    auto socket = sylar::Socket::CreateTCPSocket();
    SYLAR_ASSERT(socket);

    auto addr = sylar::Address::LookupAnyIPAddress(fileServer);
    SYLAR_ASSERT(addr);

    ret = socket->connect(addr);
    SYLAR_ASSERT(ret == true);

    SYLAR_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();
    file::STRU_FILE_CONTENT_RQ rq;
    strcpy(rq.filePath, path);
    rq.method = file::STRU_FILE_CONTENT_RQ::GET;
    socket->send((char*)&rq, sizeof(rq));
    // int rt = 0;
    int fd = open("test_received_file", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to open file for writing.";
        return;
    }
    char buffer[8192];
    ssize_t bytesRead = 0;
    ssize_t totalBytesReceived = 0;
    // 循环接收数据块
    while ((bytesRead = socket->recv(buffer, 8192)) > 0) {
        // 将接收到的数据写入文件
        ssize_t bytesWritten = write(fd, buffer, bytesRead);
        if (bytesWritten == -1) {
            SYLAR_LOG_ERROR(g_logger) << "Failed to write to file.";
            break;
        }

        totalBytesReceived += bytesRead;
    }
    close(fd);
    if (bytesRead == -1) {
        SYLAR_LOG_ERROR(g_logger) << "Failed to receive file.";
    } else {
         SYLAR_LOG_ERROR(g_logger) << "File received. Total bytes received: " << totalBytesReceived;
    }
}

void dealData(const char* recvbuf, int recvlen) {
    int header_type = *(int *)recvbuf;
    SYLAR_LOG_INFO(g_logger) << "dealData() header_type:" << header_type;
    if (header_type == _DEF_PROTOCOL_GETUSERINFO_RS) {
        dealGetInfoRs(recvbuf, recvlen);
    }
}

void recvData(sylar::Socket::ptr socket) {
    int nPackSize = 0; // 存储包大小
    int rt        = 0; // 一次接受到的字节数
    while (true) {
        rt = socket->recv((char *)&nPackSize, sizeof(int));
        SYLAR_LOG_INFO(g_logger) << "收到" << rt << "个字节, 包大小：" << nPackSize;
        int offset    = 0; // 从buf开始偏移多少
        std::string recvbuf;
        recvbuf.resize(nPackSize);
        while (nPackSize) {
            SYLAR_LOG_INFO(g_logger) << "还剩" << nPackSize << "字节未收到";
            if ((rt = socket->recv(&recvbuf[0] + offset, nPackSize)) > 0) {
                SYLAR_LOG_INFO(g_logger) << "收到" << rt << "个字节";
                nPackSize -= rt;
                offset += rt;
            }
        }
        dealData(recvbuf.c_str(), offset);
    }
}

void run() {
    bool ret;
    auto socket = sylar::Socket::CreateTCPSocket();
    SYLAR_ASSERT(socket);

    auto addr = sylar::Address::LookupAnyIPAddress(chatServer);
    SYLAR_ASSERT(addr);

    ret = socket->connect(addr);
    SYLAR_ASSERT(ret == true);

    SYLAR_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();

    chat::STRU_GET_USERINFO_RQ rq;
    rq.senderId = 4;
    strcpy(rq.senderName, "cxj");
    strcpy(rq.userName, "lsl");

    socket->send((char *)&rq, sizeof(rq));
    recvData(socket);
}

int main() {
    sylar::IOManager im(2);
    im.schedule(&run);
    return 0;
}