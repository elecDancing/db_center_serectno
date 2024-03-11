// 模拟网上银行app客户端，增加了心跳报文。
#include "_public.h"
using namespace idc;

ctcpclient tcpclient;     // tcp通讯的客户端。

string strsendbuffer;   // 发送报文的buffer。
string strrecvbuffer;    // 接收报文的buffer。

bool biz000(const int timeout);   // 心跳报文。
bool biz001(const int timeout);   // 登录。
bool biz002(const int timeout);   // 查询余额。
bool biz003(const int timeout);   // 转帐。

int main(int argc,char *argv[])
{
    if (argc!=4)
    {
      printf("Using:./demo05 ip port timeout\n");
      printf("Example:./demo05 192.168.150.128 5005 10\n\n"); 
      return -1;
    }

    if (tcpclient.connect(argv[1],atoi(argv[2]))==false)
    {
        printf ("tcpclient.connect() failed.\n"); return -1;
    }

    biz001(atoi(argv[3]));  // 登录。
    
    biz002(atoi(argv[3]));   // 查询余额。

    sleep(6);     
    biz000(atoi(argv[3]));   // 发送心跳报文。
    sleep(6);     

    biz003(atoi(argv[3]));   // 转帐。
    
}

bool biz000(const int timeout)    // 心跳报文。
{
    strsendbuffer="<bizid>0</bizid>";
    if (tcpclient.write(strsendbuffer)==false)
    {
        printf("tcpclient.write() failed.\n"); return false;
    }
    cout << "发送：" << strsendbuffer << endl;

    if (tcpclient.read(strrecvbuffer,timeout)==false)
    {
        printf("tcpclient.read() failed.\n"); return false;
    }
    cout << "接收：" << strrecvbuffer << endl;

    return true;
}

bool biz001(const int timeout)   // 登录。
{
    strsendbuffer="<bizid>1</bizid><username>13922200001</username><password>123456</password>";
    if (tcpclient.write(strsendbuffer)==false)
    {
        printf("tcpclient.write() failed.\n"); return false;
    }
    cout << "发送：" << strsendbuffer << endl;

    if (tcpclient.read(strrecvbuffer,timeout)==false)
    {
        printf("tcpclient.read() failed.\n"); return false;
    }
    cout << "接收：" << strrecvbuffer << endl;

    return true;
}

bool biz002(const int timeout)   // 查询余额。
{
    strsendbuffer="<bizid>2</bizid><cardid>6262000000001</cardid>";
    if (tcpclient.write(strsendbuffer)==false)
    {
        printf("tcpclient.write() failed.\n"); return false;
    }
    cout << "发送：" << strsendbuffer << endl;

    if (tcpclient.read(strrecvbuffer,timeout)==false)
    {
        printf("tcpclient.read() failed.\n"); return false;
    }
    cout << "接收：" << strrecvbuffer << endl;

    return true;
}

bool biz003(const int timeout)   // 转帐。
{
    strsendbuffer="<bizid>3</bizid><cardid1>6262000000001</cardid1><cardid2>6262000000001</cardid2><je>100.8</je>";
    if (tcpclient.write(strsendbuffer)==false)
    {
        printf("tcpclient.write() failed.\n"); return false;
    }
    cout << "发送：" << strsendbuffer << endl;

    if (tcpclient.read(strrecvbuffer,timeout)==false)
    {
        printf("tcpclient.read() failed.\n"); return false;
    }
    cout << "接收：" << strrecvbuffer << endl;

    return true;
}