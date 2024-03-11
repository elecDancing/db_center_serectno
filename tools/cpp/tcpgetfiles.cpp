/*
 * 程序名：tcpgetfiles.cpp，采用tcp协议，实现文件下载的客户端。
 * 作者：Prtrick。
*/
#include "_public.h"
using namespace idc;

// 程序运行的参数结构体。
struct st_arg
{
    int    clienttype;               // 客户端类型，1-上传文件；2-下载文件，本程序固定填2。
    char ip[31];                      // 服务端的IP地址。
    int    port;                        // 服务端的端口。
    char srvpath[256];           // 服务端文件存放的根目录。
	int    ptype;                      // 文件下载成功后服务端文件的处理方式：1-删除文件；2-移动到备份目录。
    char srvpathbak[256];     // 文件成功下载后，服务端文件备份的根目录，当ptype==2时有效。
    bool andchild;                 // 是否下载srvpath目录下各级子目录的文件，true-是；false-否。
    char matchname[256];    // 待下载文件名的匹配规则，如"*.TXT,*.XML"。
    char clientpath[256];       // 客户端文件存放的根目录。
    int    timetvl;                    // 扫描服务端目录文件的时间间隔，单位：秒。
    int    timeout;                  // 进程心跳的超时时间。
    char pname[51];              // 进程名，建议用"tcpgetfiles_后缀"的方式。
} starg;

clogfile logfile;

// 程序退出和信号2、15的处理函数。
void EXIT(int sig);

void _help();

// 把xml解析到参数starg结构中。
bool _xmltoarg(const char *strxmlbuffer);

ctcpclient tcpclient;

bool login(const char *argv);    // 登录业务。

string strrecvbuffer;   // 发送报文的buffer。
string strsendbuffer;   // 接收报文的buffer。

// 文件下载的主函数。
void _tcpgetfiles();

// 接收文件的内容。
bool recvfile(const string &filename,const string &mtime,int filesize);

cpactive pactive;  // 进程心跳。

int main(int argc,char *argv[])
{
    if (argc!=3) { _help(); return -1; }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    //closeioandsignal(false); 
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件。
    if (logfile.open(argv[1])==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
    }

    // 解析xml，得到程序运行的参数。
    if (_xmltoarg(argv[2])==false) return -1;

    pactive.addpinfo(starg.timeout,starg.pname);  // 把进程的心跳信息写入共享内存。

    // 向服务端发起连接请求。
    if (tcpclient.connect(starg.ip,starg.port)==false)
    {
        logfile.write("tcpclient.connect(%s,%d) failed.\n",starg.ip,starg.port); EXIT(-1);
    }

    // 登录业务。
    if (login(argv[2])==false) { logfile.write("login() failed.\n"); EXIT(-1); }

    // 调用文件下载的主函数。
    _tcpgetfiles();

    EXIT(0);
}


// 登录业务。 
bool login(const char *argv)    
{
    sformat(strsendbuffer,"%s<clienttype>2</clienttype>",argv);
    // xxxxxxxxxxxxxx logfile.write("发送：%s\n",strsendbuffer.c_str());
    if (tcpclient.write(strsendbuffer)==false) return false; // 向服务端发送请求报文。

    if (tcpclient.read(strrecvbuffer,10)==false) return false; // 接收服务端的回应报文。
    // xxxxxxxxxxxxxx logfile.write("接收：%s\n",strrecvbuffer.c_str());

    logfile.write("登录(%s:%d)成功。\n",starg.ip,starg.port); 

    return true;
}

void EXIT(int sig)
{
    logfile.write("程序退出，sig=%d\n\n",sig);

    exit(0);
}

void _help()
{
    printf("\n");
    printf("Using:/project/tools/bin/tcpgetfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 20 /project/tools/bin/tcpgetfiles /log/idc/tcpgetfiles_surfdata.log "
              "\"<ip>192.168.150.128</ip><port>5005</port>"\
              "<clientpath>/tmp/client</clientpath>"
              "<ptype>1</ptype><srvpath>/tmp/server</srvpath>"\
              "<andchild>true</andchild><matchname>*</matchname>"
              "<timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles_surfdata</pname>\"\n");

    printf("本程序是数据中心的公共功能模块，采用tcp协议从服务端下载文件。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，如下：\n");
    printf("ip            服务端的IP地址。\n");
    printf("port          服务端的端口。\n");
    printf("ptype         文件下载成功后服务端文件的处理方式：1-删除文件；2-移动到备份目录。\n");
    printf("srvpath       服务端文件存放的根目录。\n");
    printf("srvpathbak    文件成功下载后，服务端文件备份的根目录，当ptype==2时有效。\n");
    printf("andchild      是否下载srvpath目录下各级子目录的文件，true-是；false-否，缺省为false。\n");
    printf("matchname     待下载文件名的匹配规则，如\"*.TXT,*.XML\"\n");
    printf("clientpath    客户端文件存放的根目录。\n");
    printf("timetvl       扫描服务目录文件的时间间隔，单位：秒，取值在1-30之间。\n");
    printf("timeout       本程序的超时时间，单位：秒，视文件大小和网络带宽而定，建议设置50以上。\n");
    printf("pname         进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

// 把xml解析到参数starg结构
bool _xmltoarg(const char *strxmlbuffer)
{
    memset(&starg,0,sizeof(struct st_arg));

    getxmlbuffer(strxmlbuffer,"ip",starg.ip);
    if (strlen(starg.ip)==0) { logfile.write("ip is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"port",starg.port);
    if ( starg.port==0) { logfile.write("port is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"ptype",starg.ptype);
    if ((starg.ptype!=1)&&(starg.ptype!=2)) { logfile.write("ptype not in (1,2).\n"); return false; }

    getxmlbuffer(strxmlbuffer,"srvpath",starg.srvpath);
    if (strlen(starg.srvpath)==0) { logfile.write("srvpath is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"srvpathbak",starg.srvpathbak);
    if ((starg.ptype==2)&&(strlen(starg.srvpathbak)==0)) { logfile.write("srvpathbak is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"andchild",starg.andchild);

    getxmlbuffer(strxmlbuffer,"matchname",starg.matchname);
    if (strlen(starg.matchname)==0) { logfile.write("matchname is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"clientpath",starg.clientpath);
    if (strlen(starg.clientpath)==0) { logfile.write("clientpath is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"timetvl",starg.timetvl);
    if (starg.timetvl==0) { logfile.write("timetvl is null.\n"); return false; }

    // 扫描服务端目录文件的时间间隔（执行下载任务的时间间隔），单位：秒。
    // starg.timetvl没有必要超过30秒。
    if (starg.timetvl>30) starg.timetvl=30;

    // 进程心跳的超时时间，一定要大于starg.timetvl。
    getxmlbuffer(strxmlbuffer,"timeout",starg.timeout);
    if (starg.timeout==0) { logfile.write("timeout is null.\n"); return false; }
    if (starg.timeout<=starg.timetvl)  { logfile.write("starg.timeout(%d) <= starg.timetvl(%d).\n",starg.timeout,starg.timetvl); return false; }

    getxmlbuffer(strxmlbuffer,"pname",starg.pname,50);
    //if (strlen(starg.pname)==0) { logfile.write("pname is null.\n"); return false; }

    return true;
}

// 文件下载的主函数。
void _tcpgetfiles()
{
    while (true)
    {
        pactive.uptatime();

        // 接收服务端的报文。
        if (tcpclient.read(strrecvbuffer,starg.timetvl+10)==false)
        {
            logfile.write("tcpclient.read() failed.\n"); return;
        }
        // logfile.write("strrecvbuffer=%s\n",strrecvbuffer.c_str());

        // 处理心跳报文。
        if (strrecvbuffer=="<activetest>ok</activetest>")
        {
            strsendbuffer="ok";
            // xxxxxxxxxxx logfile.write("strsendbuffer=%s\n",strsendbuffer.c_str());
            if (tcpclient.write(strsendbuffer)==false)
            {
                logfile.write("tcpclient.write() failed.\n"); return;
            }
        }

        // 处理下载文件的请求报文。
        if (strrecvbuffer.find("<filename>") != string::npos)
        {
            // 解析下载文件请求报文的xml。
            string serverfilename;
            string mtime; 
            int  filesize=0;
            getxmlbuffer(strrecvbuffer,"filename",serverfilename);
            getxmlbuffer(strrecvbuffer,"mtime",mtime);
            getxmlbuffer(strrecvbuffer,"size",filesize);

            // 客户端和服务端文件的目录是不一样的，以下代码生成客户端的文件名。
            // 把文件名中的srvpath替换成clientpath，要小心第三个参数
            string clientfilename;
            clientfilename=serverfilename;
            replacestr(clientfilename,starg.srvpath,starg.clientpath,false);

            // 接收文件的内容。
            logfile.write("recv %s(%d) ...",clientfilename.c_str(),filesize);
            if (recvfile(clientfilename,mtime,filesize)==true)
            {
                logfile << "ok.\n";
                sformat(strsendbuffer,"<filename>%s</filename><result>ok</result>",serverfilename.c_str());
            }
            else
            {
                logfile << "failed.\n";
                sformat(strsendbuffer,"<filename>%s</filename><result>failed</result>",serverfilename.c_str());
            }

            // 把接收结果返回给对端。
            // xxxxxxxxxxx logfile.write("strsendbuffer=%s\n",strsendbuffer.c_str());
            if (tcpclient.write(strsendbuffer)==false)
            {
                logfile.write("tcpclient.write() failed.\n"); return;
            }
        }
    }
}

// 接收文件的内容。
bool recvfile(const string &filename,const string &mtime,int filesize)
{
    int  totalbytes=0;        // 已接收文件的总字节数。
    int  onread=0;            // 本次打算接收的字节数。
    char buffer[1000];        // 接收文件内容的缓冲区。
    cofile ofile;

    newdir(filename);

    if (ofile.open(filename,true,ios::out|ios::binary)==false) return false;

    while (true)
    {
        memset(buffer,0,sizeof(buffer));

        // 计算本次应该接收的字节数。
        if (filesize-totalbytes>1000) onread=1000;
        else onread=filesize-totalbytes;

        // 接收文件内容。
        if (tcpclient.read(buffer,onread)==false)  return false; 

        // 把接收到的内容写入文件。
        ofile.write(buffer,onread);

        // 计算已接收文件的总字节数，如果文件接收完，跳出循环。
        totalbytes=totalbytes+onread;

        if (totalbytes==filesize) break;
    }

    ofile.closeandrename();

    // 重置文件的时间。
    setmtime(filename,mtime);

    return true;
}