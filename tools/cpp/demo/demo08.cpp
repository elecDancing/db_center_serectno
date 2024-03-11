// 测试同步/异步通讯的服务端程序。
#include "_public.h"
using namespace idc;

int main(int argc,char *argv[])
{
    if (argc!=2)  { cout << "Using: ./demo08 port\n";  return -1;}

    ctcpserver tcpserver;
    if (tcpserver.initserver(atoi(argv[1]))==false)
    {
        printf("tcpserver.initserver() failed.\n"); return -1;
    }

    tcpserver.accept();

    string strsendbuffer,strrecvbuffer;
    while (true)
    {
        if (tcpserver.read(strrecvbuffer)==false) break;    // 接收客户端的报文。

        sformat(strsendbuffer,"回复：%s",strrecvbuffer.c_str());
        if (tcpserver.write(strsendbuffer)==false) break;  // 向客户发送回应报文。
    }
}
