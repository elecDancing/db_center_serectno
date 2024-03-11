// 模拟网上银行app服务端。
#include "_public.h"
using namespace idc;

ctcpserver tcpserver;  // 创建服务端对象。
clogfile logfile;            // 服务程序的运行日志。

void FathEXIT(int sig);  // 父进程退出函数。
void ChldEXIT(int sig);  // 子进程退出函数。

string strsendbuffer;   // 发送报文的buffer。
string strrecvbuffer;    // 接收报文的buffer。

bool bizmain();    // 业务处理主函数。

int main(int argc,char *argv[])
{
    if (argc!=3)
    {
      printf("Using:./demo04 port logfile\n");
      printf("Example:./demo04 5005 /log/idc/demo04.log\n\n"); 
      return -1;
    }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程
    // 但请不要用 "kill -9 +进程号" 强行终止
    //closeioandsignal(false); 
    signal(SIGINT,FathEXIT); signal(SIGTERM,FathEXIT);

    if (logfile.open(argv[2])==false) { printf("logfile.open(%s) failed.\n",argv[2]); return -1; }

    // 服务端初始化。
    if (tcpserver.initserver(atoi(argv[1]))==false)
    {
      logfile.write("tcpserver.initserver(%s) failed.\n",argv[1]); return -1;
    }

    while (true)
    {
        // 获取客户端的连接请求。
        if (tcpserver.accept()==false)
        {
            logfile.write("tcpserver.accept() failed.\n"); FathEXIT(-1);
        }

        logfile.write("客户端（%s）已连接。\n",tcpserver.getip());

        if (fork()>0) { tcpserver.closeclient(); continue; }  // 父进程继续回到accept()。
   
        // 子进程重新设置退出信号。
        signal(SIGINT,ChldEXIT); signal(SIGTERM,ChldEXIT);

        tcpserver.closelisten();     // 子进程关闭监听的socket。

        while (true)
        {
            // 子进程与客户端进行通讯，处理业务。
            if (tcpserver.read(strrecvbuffer)==false)
            {
                logfile.write("tcpserver.read() failed.\n"); ChldEXIT(0);
            }
            logfile.write("接收：%s\n",strrecvbuffer.c_str());

            bizmain();    // 业务处理主函数。

            if (tcpserver.write(strsendbuffer)==false)
            {
                logfile.write("tcpserver.send() failed.\n"); ChldEXIT(0);
            }
            logfile.write("发送：%s\n",strsendbuffer.c_str());
        }

        ChldEXIT(0);
    }
}

// 父进程退出函数。
void FathEXIT(int sig)  
{
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断。
    signal(SIGINT,SIG_IGN); signal(SIGTERM,SIG_IGN);

    logfile.write("父进程退出，sig=%d。\n",sig);

    tcpserver.closelisten();    // 关闭监听的socket。

    kill(0,15);     // 通知全部的子进程退出。

    exit(0);
}

// 子进程退出函数。
void ChldEXIT(int sig)  
{
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断。
    signal(SIGINT,SIG_IGN); signal(SIGTERM,SIG_IGN);

    logfile.write("子进程退出，sig=%d。\n",sig);

    tcpserver.closeclient();    // 关闭客户端的socket。

    exit(0);
}

void biz001();   // 登录。
void biz002();   // 查询余额。
void biz003();   // 转帐。

bool bizmain()    // 业务处理主函数。
{
    int bizid;  // 业务代码。
    getxmlbuffer(strrecvbuffer,"bizid",bizid);

    switch(bizid)
    {
        case 1:    // 登录。
            biz001();
            break;
        case 2:    // 查询余额。
            biz002();
            break;
        case 3:    // 转帐。
            biz003();
            break;
        default:   // 非法报文。
            strsendbuffer="<retcode>9</retcode><message>业务不存在。</message>";
            break;
    }

    return true;
}

void biz001()   // 登录。
{
    string username,password;
    getxmlbuffer(strrecvbuffer,"username",username);
    getxmlbuffer(strrecvbuffer,"password",password);

    if ( (username=="13922200000") && (password=="123456") )
        strsendbuffer="<retcode>0</retcode><message>成功。</message>";
    else
        strsendbuffer="<retcode>-1</retcode><message>用户名或密码不正确。</message>";
}

void biz002()   // 查询余额。
{
    string cardid;
    getxmlbuffer(strrecvbuffer,"cardid",cardid);  // 获取卡号。

    // 假装操作了数据库，得到了卡的余额。

    strsendbuffer="<retcode>0</retcode><ye>128.83</ye>";
}

void biz003()   // 转帐。
{
    string cardid1,cardid2;
    getxmlbuffer(strrecvbuffer,"cardid1",cardid1);
    getxmlbuffer(strrecvbuffer,"cardid2",cardid2);
    double je;
    getxmlbuffer(strrecvbuffer,"je",je);

    // 假装操作了数据库，更新了两个账户的金额，完成了转帐操作。

    if ( je<100 )
        strsendbuffer="<retcode>0</retcode><message>成功。</message>";
    else
        strsendbuffer="<retcode>-1</retcode><message>余额不足。</message>";
}