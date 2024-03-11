// 测试同步通讯的客户端程序。
#include "_public.h"
using namespace idc;

int main(int argc,char *argv[])
{
    if (argc!=3)  { cout << "Using: ./demo07 ip port\n";  return -1;}

    ctcpclient tcpclient;
    if (tcpclient.connect(argv[1],atoi(argv[2]))==false)
    {
        printf("tcpclient.connect failed.\n"); return -1;        
    }

    clogfile logfile;
    logfile.open("/tmp/demo07.log");

    string strsendbuffer,strrecvbuffer;

    for (int ii=1;ii<=100;ii++)
    {
        sformat(strsendbuffer,"这是第%d个超级女生。",ii);
        logfile.write("%s\n",strsendbuffer.c_str());
        tcpclient.write(strsendbuffer);           // 向服务端发送报文。

        tcpclient.read(strrecvbuffer);             // 等待服务端的回应。
        logfile.write("%s\n",strrecvbuffer.c_str());
    }
}