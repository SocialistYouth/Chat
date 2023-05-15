/*
 * @Author: Cui XiaoJun
 * @Date: 2023-05-13 15:09:17
 * @LastEditTime: 2023-05-13 22:34:12
 * @email: cxj2856801855@gmail.com
 * @github: https://github.com/SocialistYouth/
 */
#include "chat/chatserver.h"


int main(int argc, char const *argv[])
{
    sylar::IOManager iom(2);
    chat::ChatServer::ptr server(new chat::ChatServer);
    iom.schedule(std::bind(&chat::ChatServer::init, server));
    return 0;
}
