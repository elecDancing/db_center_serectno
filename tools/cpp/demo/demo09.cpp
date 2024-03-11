// 多进程版本的异步通讯的客户端程序。
#include "_public.h"
using namespace idc;

int main(int argc,char *argv[])
{
    if (argc!=3)  { cout << "Using: ./demo09 ip port\n";  return -1;}

    ctcpclient tcpclient;
    if (tcpclient.connect(argv[1],atoi(argv[2]))==false)
    {
        printf("tcpclient.connect failed.\n"); return -1;        
    }

    clogfile logfile;
    logfile.open("/tmp/demo09.log");

    if (fork()==0)
    {
        string strsendbuffer;

        for (int ii=1;ii<=1000000;ii++)
        {
            sformat(strsendbuffer,"这是第%d个超级女生。",ii);
            logfile.write("%s\n",strsendbuffer.c_str());
            tcpclient.write(strsendbuffer);
        }
    }
    else
    {
        string strrecvbuffer;

        for (int ii=1;ii<=1000000;ii++)
        {
            tcpclient.read(strrecvbuffer);
            logfile.write("%s\n",strrecvbuffer.c_str());
        }
    }
}