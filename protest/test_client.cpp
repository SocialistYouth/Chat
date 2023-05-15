#include "sylar/socket.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/macro.h"
#include "chat/protocol.h"
#include <stdlib.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

const char *ip = "0.0.0.0:8080";

void run() {
    int ret;

    auto socket = sylar::Socket::CreateTCPSocket();
    SYLAR_ASSERT(socket);

    auto addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8080");
    SYLAR_ASSERT(addr);

    ret = socket->connect(addr);
    SYLAR_ASSERT(ret == true);

    SYLAR_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();
    
    chat::STRU_ADD_FRIEND_RS rq;
    
    strcpy(rq.friendName, "test");
    rq.friendid = 3;
    rq.userid   = 5;
    rq.result   = add_success;
    socket->send((char *)&rq, sizeof(rq));
    SYLAR_LOG_INFO(g_logger) << "send: 包大小" << sizeof(rq);
    int nPackSize = 0;	// 存储包大小
    int rt        = 0;  // 一次接受到的字节数
    while (true) {
        rt = socket->recv((char*)&nPackSize, sizeof(int));
        SYLAR_LOG_INFO(g_logger) << "收到" << rt << "个字节, 包大小：" << nPackSize;
        if (rt <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "client error rt=" << rt
                                     << " errno=" << errno << " errstr=" << strerror(errno);
			break;
        }
        int offset = 0; // 从buf开始偏移多少
        char *recvbuf = new char[nPackSize];
        std::string buf;
        buf.resize(nPackSize);
        while (nPackSize) {
            SYLAR_LOG_INFO(g_logger) << "还剩" << nPackSize << "字节未收到";
            if ((rt = socket->recv(&buf[0] + offset, sizeof(recvbuf))) > 0) {
                SYLAR_LOG_INFO(g_logger) << "收到" << rt << "个字节";
				nPackSize -= rt;
				offset += rt;
            }
        }
        recvbuf[offset] = '\0';
        SYLAR_LOG_INFO(g_logger) << "recv: " << buf;
        // dealData(client, recvbuf, offset);
    }
    return;
}

int main() {
    sylar::IOManager im(2);
    im.schedule(&run);
    return 0;
}