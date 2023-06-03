#include "chat/chatserver.h"
#include "chat/fileserver.h"

int main() {
    sylar::IOManager iom(5);
    chat::ChatServer::ptr chatServer(new chat::ChatServer);
    file::FileServer::ptr fileServer(new file::FileServer);
    
    iom.schedule(std::bind(&chat::ChatServer::init, chatServer));
    iom.schedule(std::bind(&file::FileServer::init, fileServer));
    return 0;
}