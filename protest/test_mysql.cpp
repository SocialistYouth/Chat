#include <chat/MySQLConnection.h>
#include <iostream>
using namespace std;

int main() {
    mysql::CMySql sql;
    if (!sql.ConnectMySql("127.0.0.1", "cxj", "Becauseofyou0926!", "IM"))
        cout << "connect mysql failed" << endl;
    string sqlbuf = "show tables;";
    list<string> resbuf;
    sql.GetTables(sqlbuf.c_str(), resbuf);
    for (auto i : resbuf)
        cout << i << endl;
    resbuf.clear();
    sqlbuf.clear();
    sqlbuf = "select * from test;";
    sql.SelectMySql(sqlbuf.c_str(), 1, resbuf);
    for (auto i : resbuf)
        cout << i << endl;
    sql.DisConnect();
    return 0;
}