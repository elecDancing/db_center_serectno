// I/O复用版本的异步通讯的客户端程序。
#include "_public.h"
using namespace idc;

int main(int argc,char *argv[])
{
    if (argc!=3)  { cout << "Using: ./demo10 ip port\n";  return -1;}

    ctcpclient tcpclient;
    if (tcpclient.connect(argv[1],atoi(argv[2]))==false)
    {
        printf("tcpclient.connect failed.\n"); return -1;        
    }

    clogfile logfile;
    logfile.open("/tmp/demo10.log");

    string strsendbuffer,strrecvbuffer;

    int ack=0;       // 已接收回应报文的计数器。

    for (int ii=1;ii<=1000000;ii++)
    {
        sformat(strsendbuffer,"这是第%d个超级女生。",ii);
        logfile.write("%s\n",strsendbuffer.c_str());
        tcpclient.write(strsendbuffer);           // 向服务端发送报文。

        while (tcpclient.read(strrecvbuffer,-1)==true)      // 检查tcp的缓冲区中是否有服务端回应报文。
        { 
            logfile.write("%s\n",strrecvbuffer.c_str());
            ack++;
        }
    }

    while (ack<1000000)
    {
        if (tcpclient.read(strrecvbuffer)==true)              // 等待服务端回应报文。
        { 
            logfile.write("%s\n",strrecvbuffer.c_str());
            ack++;
        }
    }
}