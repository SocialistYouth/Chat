/**
 * @file test_socket_tcp_client.cc
 * @brief 测试Socket类，tcp客户端
 * @version 0.1
 * @date 2021-09-18
 */
#include<sylar/sylar.h>
#include <iostream>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_tcp_client() {
    int ret;

    auto socket = sylar::Socket::CreateTCPSocket();
    SYLAR_ASSERT(socket);

    auto addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8080");
    SYLAR_ASSERT(addr);

    ret = socket->connect(addr);
    SYLAR_ASSERT(ret);

    SYLAR_LOG_INFO(g_logger) << "connect success, peer address: " << socket->getRemoteAddress()->toString();
    while(1) {
        std::string buffer;
        std::cout << "请输入: ";
        std::cin >> buffer;
        socket->send(buffer.c_str(), strlen(buffer.c_str()));
        SYLAR_LOG_INFO(g_logger) << "send: " << buffer;
    }
    // socket->close();

    return;
}

int main(int argc, char *argv[]) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir(sylar::EnvMgr::GetInstance()->getConfigPath());

    sylar::IOManager iom;
    iom.schedule(&test_tcp_client);

    return 0;
}