#include "_public.h"
#include "_ftp.h"

using namespace idc;

// 程序退出和信号2、15的处理函数。
void EXIT(int sig);

// 程序运行参数的结构体。
struct st_arg
{
    char host[31];                        // 远程服务端的IP和端口。
    int    mode;                           // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    char username[31];               // 远程服务端ftp的用户名。
    char password[31];                // 远程服务端ftp的密码。
    char remotepath[256];          // 远程服务端存放文件的目录。
    char localpath[256];              // 本地文件存放的目录。
    char matchname[256];          // 待下载文件匹配的规则。
    int   ptype;                            // 下载后服务端文件的处理方式：1-什么也不做；2-删除；3-备份。
    char remotepathbak[256];   // 下载后服务端文件的备份目录。
} starg;

bool _xmltoarg(const char *strxmlbuffer);  // 把xml解析到参数starg结构中。

clogfile logfile;
cftpclient ftp;       // 创建ftp客户端对象。

void _help();        // 显示帮助文档。

struct st_fileinfo              // 文件信息的结构体。
{
    string filename;           // 文件名。
    string mtime;              // 文件时间。
    st_fileinfo()=default;
    st_fileinfo(const string &in_filename,const string &in_mtime):filename(in_filename),mtime(in_mtime) {}
    void clear() { filename.clear(); mtime.clear(); }
}; 

vector<struct st_fileinfo> vfilelist;      // 下载前列出服务端文件名的容器，从nlist文件中加载。

bool loadlistfile();            // 把ftpclient.nlist()方法获取到的list文件加载到容器vfilelist中。

int main(int argc,char *argv[])
{
    // 第一步计划：从服务器某个目录中下载文件，可以指定文件名匹配的规则。
    if (argc!=3) { _help();    return -1; }

    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    // closeioandsignal(true);       // 关闭0、1、2和忽略全部的信号，在调试阶段，这行代码可以不启用。
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件。
    if (logfile.open(argv[1])==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
    }

    // 解析xml，得到程序运行的参数。
    if (_xmltoarg(argv[2])==false) return -1;

    // 登录ftp服务器。
    if (ftp.login(starg.host,starg.username,starg.password,starg.mode)==false)
    {
        logfile.write("ftp.login(%s,%s,%s) failed.\n%s\n",starg.host,starg.username,starg.password,ftp.response()); return -1;
    }

    logfile.write("ftp.login ok.\n");

    // 进入ftp服务器存放文件的目录。
    if (ftp.chdir(starg.remotepath)==false)
    {
        logfile.write("ftp.chdir(%s) failed.\n%s\n",starg.remotepath,ftp.response()); return -1;
    }

    // 调用ftpclient.nlist()方法列出服务器目录中的文件名，保存在本地文件中。
    if (ftp.nlist(".",sformat("/tmp/nlist/ftpgetfiles_%d.nlist",getpid())) == false)
    {
        logfile.write("ftp.nlist(%s) failed.\n%s\n",starg.remotepath,ftp.response()); return -1;
    }
    logfile.write("nlist(%s) ok.\n",sformat("/tmp/nlist/ftpgetfiles_%d.nlist",getpid()).c_str());

    // 把ftpclient.nlist()方法获取到的list文件加载到容器vfilelist中。
    if (loadlistfile()==false)
    {
      logfile.write("loadlistfile() failed.\n");  return -1;
    }

    string strremotefilename,strlocalfilename;

    // 遍历vfilelist容器。
    for (auto & aa : vfilelist) 
    {
        sformat(strremotefilename,"%s/%s",starg.remotepath,aa.filename.c_str());         // 拼接服务端全路径的文件名。
        sformat(strlocalfilename,"%s/%s",starg.localpath,aa.filename.c_str());                 // 拼接本地全路径的文件名。

        logfile.write("get %s ...",strremotefilename.c_str());
        // 调用ftpclient.get()方法下载文件。
        if (ftp.get(strremotefilename,strlocalfilename)==false) 
        {
            logfile << "failed.\n" << ftp.response() << "\n"; return -1;
        }

        logfile << "ok.\n"; 

        // ptype==1，增量下载文件。
        // if (starg.ptype==1) {}

        // ptype==2，删除服务端的文件。
        if (starg.ptype==2)
        {
            if (ftp.ftpdelete(strremotefilename)==false)
            {
                logfile.write("ftp.ftpdelete(%s) failed.\n%s\n",strremotefilename.c_str(),ftp.response()); return -1;
            }
        }

        // ptype==3，把服务端的文件移动到备份目录。
        if (starg.ptype==3)
        {
            string strremotefilenamebak=sformat("%s/%s",starg.remotepathbak,aa.filename.c_str());  // 生成全路径的备份文件名。
            if (ftp.ftprename(strremotefilename,strremotefilenamebak)==false)
            {
                logfile.write("ftp.ftprename(%s,%s) failed.\n%s\n",strremotefilename.c_str(),strremotefilenamebak.c_str(),ftp.response()); return -1;
            }
        }
    }

    return 0;
}

void _help()        // 显示帮助文档。
{
    printf("\n");
    printf("Using:/project/tools/bin/ftpgetfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 30 /project/tools/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log " \
              "\"<host>192.168.150.128:21</host><mode>1</mode>"\
              "<username>wucz</username><password>oracle</password>"\
              "<remotepath>/tmp/idc/surfdata</remotepath><localpath>/idcdata/surfdata</localpath>"\
              "<matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname>"\
              "<ptype>3</ptype><remotepathbak>/tmp/idc/surfdatabak</remotepathbak>\"\n\n");

    printf("本程序是通用的功能模块，用于把远程ftp服务端的文件下载到本地目录。\n");
    printf("logfilename是本程序运行的日志文件。\n");
    printf("xmlbuffer为文件下载的参数，如下：\n");
    printf("<host>192.168.150.128:21</host> 远程服务端的IP和端口。\n");
    printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("<username>wucz</username> 远程服务端ftp的用户名。\n");
    printf("<password>oraccle</password> 远程服务端ftp的密码。\n");
    printf("<remotepath>/tmp/idc/surfdata</remotepath> 远程服务端存放文件的目录。\n");
    printf("<localpath>/idcdata/surfdata</localpath> 本地文件存放的目录。\n");
    printf("<matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname> 待下载文件匹配的规则。"\
              "不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("<ptype>1</ptype> 文件下载成功后，远程服务端文件的处理方式："\
              "1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("<remotepathbak>/tmp/idc/surfdatabak</remotepathbak> 文件下载成功后，服务端文件的备份目录，"\
              "此参数只有当ptype=3时才有效。\n\n\n");
}

// 把xml解析到参数starg结构中。
bool _xmltoarg(const char *strxmlbuffer)
{
    memset(&starg,0,sizeof(struct st_arg));

    getxmlbuffer(strxmlbuffer,"host",starg.host,30);   // 远程服务端的IP和端口。
    if (strlen(starg.host)==0)
    { logfile.write("host is null.\n");  return false; }

    getxmlbuffer(strxmlbuffer,"mode",starg.mode);   // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    if (starg.mode!=2)  starg.mode=1;

    getxmlbuffer(strxmlbuffer,"username",starg.username,30);   // 远程服务端ftp的用户名。
    if (strlen(starg.username)==0)
    { logfile.write("username is null.\n");  return false; }

    getxmlbuffer(strxmlbuffer,"password",starg.password,30);   // 远程服务端ftp的密码。
    if (strlen(starg.password)==0)
    { logfile.write("password is null.\n");  return false; }

    getxmlbuffer(strxmlbuffer,"remotepath",starg.remotepath,255);   // 远程服务端存放文件的目录。
    if (strlen(starg.remotepath)==0)
    { logfile.write("remotepath is null.\n");  return false; }

    getxmlbuffer(strxmlbuffer,"localpath",starg.localpath,255);   // 本地文件存放的目录。
    if (strlen(starg.localpath)==0)
    { logfile.write("localpath is null.\n");  return false; }

    getxmlbuffer(strxmlbuffer,"matchname",starg.matchname,100);   // 待下载文件匹配的规则。
    if (strlen(starg.matchname)==0)
    { logfile.write("matchname is null.\n");  return false; }  

    // 下载后服务端文件的处理方式：1-什么也不做；2-删除；3-备份。
    getxmlbuffer(strxmlbuffer,"ptype",starg.ptype);   
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    { logfile.write("ptype is error.\n"); return false; }

    // 下载后服务端文件的备份目录。
    if (starg.ptype==3) 
    {
        getxmlbuffer(strxmlbuffer,"remotepathbak",starg.remotepathbak,255); 
        if (strlen(starg.remotepathbak)==0) { logfile.write("remotepathbak is null.\n");  return false; }
    }

    return true;
}

void EXIT(int sig)
{
    printf("程序退出，sig=%d\n\n",sig);

    exit(0);
}

// 把ftp.nlist()方法获取到的list文件加载到容器vfilelist中。
bool loadlistfile()
{
    vfilelist.clear();

    cifile  ifile;
    if (ifile.open(sformat("/tmp/nlist/ftpgetfiles_%d.nlist",getpid()))==false)
    {
      logfile.write("ifile.open(%s) 失败。\n",sformat("/tmp/nlist/ftpgetfiles_%d.nlist",getpid())); return false;
    }

    string strfilename;

    while (true)
    {
        if (ifile.readline(strfilename)==false) break;

        if (matchstr(strfilename,starg.matchname)==false) continue;

        vfilelist.emplace_back(strfilename,"");
    }

    ifile.closeandremove();

    //for (auto &aa:vfilelist)
    //    logfile.write("filename=%s=\n",aa.filename.c_str());

    return true;
}