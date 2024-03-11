/*
 *  程序名：demo47.cpp，此程序演示采用开发框架的ctcpserver类传输二进制数据。（网络通讯的服务端）
 *  作者：Prtrick
*/
#include "../_public.h"
using namespace std;
using namespace idc;

int main(int argc,char *argv[])
{
    if (argc!=2)
    {
        printf("Using:./demo47 port\n");
        printf("Sample:./demo47 5005\n");
        return -1;
    }

    ctcpserver tcpserver;
    if (tcpserver.initserver(atoi(argv[1]))==false)        // 服务端初始化。
    {
        printf("tcpserver.initserver(%s) failed.\n",argv[1]); return -1;
    }

    if (tcpserver.accept()==false)                                // 等待客户端的连接。
    {
        printf("accept() failed.\n"); return -1;
    }
    cout << "客户端已连接(" << tcpserver.getip() << ")。\n";

    struct st_girl
    {
        int bh;
        char name[31];
    }stgirl;

    string sendbuf;

    while (true)
    {
        if (tcpserver.read(&stgirl,sizeof(stgirl))==false)          // 接收客户端的请求数据。
        {
            printf("tcpserver.read() failed.\n"); break;
        }
        cout << sformat("接收：bh=%d,name=%s\n",stgirl.bh,stgirl.name);

        sendbuf="ok";
        if (tcpserver.write("ok")==false)        // 向客户端发送回应报文。
        {
            printf("tcpserver.write() failed.\n"); break;
        }
        cout << "发送：" << sendbuf << endl;
    }
}