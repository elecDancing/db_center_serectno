#include "_public.h"
#include "_ftp.h"
using namespace idc;

// 程序运行参数的结构体。
struct st_arg
{
    char host[31];                    // 远程服务端的IP和端口。
    int  mode;                         // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    char username[31];           // 远程服务端ftp的用户名。
    char password[31];           // 远程服务端ftp的密码。
    char remotepath[256];     // 远程服务端存放文件的目录。
    char localpath[256];         // 本地文件存放的目录。
    char matchname[101];     // 待上传文件匹配的规则。
    int  ptype;                        // 上传后客户端文件的处理方式：1-什么也不做；2-删除；3-备份。
    char localpathbak[256];   // 上传后客户端文件的备份目录。
    char okfilename[256];      // 已上传成功文件名清单。
    int  timeout;                     // 进程心跳的超时时间。
    char pname[51];               // 进程名，建议用"ftpputfiles_后缀"的方式。
} starg;

// 文件信息的结构体。
struct st_fileinfo
{
    string filename;
    string mtime;
    st_fileinfo()=default;
    st_fileinfo(const string &in_filename,const string &in_mtime):filename(in_filename),mtime(in_mtime) {}
    void clear() { filename.clear(); mtime.clear(); }
};

map<string,string>     mfromok;     // 已上传成功文件，从starg.okfilename中加载。
list<struct st_fileinfo> vfromdir;      // 客户端目录中的文件名。
list<struct st_fileinfo> vtook;           // 本次不需要上传的文件。
list<struct st_fileinfo> vupload;       // 本次需要上传的文件。

clogfile logfile;     // 日志文件对象。
cftpclient ftp;       // 创建ftp客户端对象。
cpactive pactive;  // 进程心跳的对象。

// 把starg.localpath目录下的文件列表加载到vfromdir容器中。
bool loadlocalfile();

// 加载starg.okfilename文件中的内容到容器vfromok中。
bool loadokfile();

bool compmap();    // 比较vfromdir和mfromok，得到vtook和vupload。

// 把容器vtook中的内容写入starg.okfilename文件，覆盖之前的旧starg.okfilename文件。
bool writetookfile();

// 如果ptype==1，把上传成功的文件记录追加到starg.okfilename文件中。
bool appendtookfile(struct st_fileinfo &stfileinfo);

// 程序退出和信号2、15的处理函数。
void EXIT(int sig);

void _help();

// 把xml解析到参数starg结构中。
bool _xmltoarg(char *strxmlbuffer);

int main(int argc,char *argv[])
{
    if (argc!=3) { _help(); return -1; }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    //closeioandsignal(true); 
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件。
    if (logfile.open(argv[1])==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
    }

    // 解析xml，得到程序运行的参数。
    if (_xmltoarg(argv[2])==false) return -1;

    pactive.addpinfo(starg.timeout,starg.pname);  // 把进程的心跳信息写入共享内存。

    // 登录ftp服务端。
    if (ftp.login(starg.host,starg.username,starg.password,starg.mode)==false)
    {
        logfile.write("ftp.login(%s,%s,%s) failed.\n%s\n",starg.host,starg.username,starg.password,ftp.response()); return -1;
    }

    // logfile.write("ftp.login ok.\n");  // 正式运行后，可以注释这行代码。

    // 把starg.localpath目录下的文件列表加载到vfromdir容器中。
    if (loadlocalfile()==false)
    {
        logfile.write("loadlocalfile() failed.\n");  return -1;
    }

    pactive.uptatime();   // 更新进程的心跳。

    if (starg.ptype==1)
    {
        loadokfile();

        // 比较vfromdir和vfromok，得到vtook和vupload。
        compmap();

        // 把容器vtook中的内容写入starg.okfilename文件，覆盖之前的旧starg.okfilename文件。
        writetookfile();
    }
    else
        vfromdir.swap(vupload);

    pactive.uptatime();   // 更新进程的心跳。

    string strremotefilename,strlocalfilename;

    // 遍历容器vupload。
    for (auto &aa:vupload)
    {
        sformat(strremotefilename,"%s/%s",starg.remotepath,aa.filename.c_str());
        sformat(strlocalfilename,"%s/%s",starg.localpath,aa.filename.c_str());

        logfile.write("put %s ...",strlocalfilename.c_str());

        // 调用ftp.put()方法把文件上传到服务端，第三个参数填true的目的是确保文件上传成功，对方不可抵赖。
        if (ftp.put(strlocalfilename,strremotefilename,true)==false) 
        {
            logfile << "failed.\n" << ftp.response() << "\n"; return -1; 
        }

        logfile << "ok.\n";  

        pactive.uptatime();   // 更新进程的心跳。
    
        // 如果ptype==1，把上传成功的文件记录追加到starg.okfilename文件中。
        if (starg.ptype==1) appendtookfile(aa);

        // 删除文件。
        if (starg.ptype==2)
        {
            if (remove(strlocalfilename.c_str())!=0)
            {
                logfile.write("remove(%s) failed.\n",strlocalfilename.c_str()); return -1;
            }
        }

        // 转存到备份目录。
        if (starg.ptype==3)
        {
            string strlocalfilenamebak=sformat("%s/%s",starg.localpathbak,aa.filename.c_str());
            if (renamefile(strlocalfilename,strlocalfilenamebak)==false)
            {
                logfile.write("renamefile(%s,%s) failed.\n",strlocalfilename.c_str(),strlocalfilenamebak.c_str()); return -1;
            }
        }
    }

    return 0;
}

void EXIT(int sig)
{
    printf("程序退出，sig=%d\n\n",sig);

    exit(0);
}

void _help()
{
    printf("\n");
    printf("Using:/project/tools/bin/ftpputfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 30 /project/tools/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log "\
              "\"<host>127.0.0.1:21</host><mode>1</mode><username>wucz</username><password>oracle</password>"\
              "<localpath>/tmp/idc/surfdata</localpath><remotepath>/idcdata/surfdata</remotepath>"\
              "<matchname>SURF_ZH*.JSON</matchname>"\
              "<ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak>"\
              "<okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename>"\
              "<timeout>80</timeout><pname>ftpputfiles_surfdata</pname>\"\n\n\n");

    printf("本程序是通用的功能模块，用于把本地目录中的文件上传到远程的ftp服务器。\n");
    printf("logfilename是本程序运行的日志文件。\n");
    printf("xmlbuffer为文件上传的参数，如下：\n");
    printf("<host>127.0.0.1:21</host> 远程服务端的IP和端口。\n");
    printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("<username>wucz</username> 远程服务端ftp的用户名。\n");
    printf("<password>wuczpwd</password> 远程服务端ftp的密码。\n");
    printf("<remotepath>/tmp/ftpputest</remotepath> 远程服务端存放文件的目录。\n");
    printf("<localpath>/tmp/idc/surfdata</localpath> 本地文件存放的目录。\n");
    printf("<matchname>SURF_ZH*.JSON</matchname> 待上传文件匹配的规则。"\
           "不匹配的文件不会被上传，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("<ptype>1</ptype> 文件上传成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("<localpathbak>/tmp/idc/surfdatabak</localpathbak> 文件上传成功后，本地文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("<okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename> 已上传成功文件名清单，此参数只有当ptype=1时才有效。\n");
    printf("<timeout>80</timeout> 上传文件超时时间，单位：秒，视文件大小和网络带宽而定。\n");
    printf("<pname>ftpputfiles_surfdata</pname> 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}

// 把xml解析到参数starg结构中。
bool _xmltoarg(char *strxmlbuffer)
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

    getxmlbuffer(strxmlbuffer,"matchname",starg.matchname,100);   // 待上传文件匹配的规则。
    if (strlen(starg.matchname)==0)
    { logfile.write("matchname is null.\n");  return false; }

    // 上传后客户端文件的处理方式：1-什么也不做；2-删除；3-备份。
    getxmlbuffer(strxmlbuffer,"ptype",starg.ptype);   
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    { logfile.write("ptype is error.\n"); return false; }

    if (starg.ptype==3)
    {
        getxmlbuffer(strxmlbuffer,"localpathbak",starg.localpathbak,255); // 上传后客户端文件的备份目录。
        if (strlen(starg.localpathbak)==0) { logfile.write("localpathbak is null.\n");  return false; }
    }

    if (starg.ptype==1)
    {
        getxmlbuffer(strxmlbuffer,"okfilename",starg.okfilename,255); // 已上传成功文件名清单。
        if (strlen(starg.okfilename)==0) { logfile.write("okfilename is null.\n");  return false; }
    }

    getxmlbuffer(strxmlbuffer,"timeout",starg.timeout);   // 进程心跳的超时时间。
    if (starg.timeout==0) { logfile.write("timeout is null.\n");  return false; }

    getxmlbuffer(strxmlbuffer,"pname",starg.pname,50);     // 进程名。
    //if (strlen(starg.pname)==0) { logfile.write("pname is null.\n");  return false; }

    return true;
}

// 把starg.localpath目录下的文件列表加载到vfromdir容器中。
bool loadlocalfile()
{
    vfromdir.clear();

    cdir dir;

    // 不包括子目录。
    if (dir.opendir(starg.localpath,starg.matchname)==false)
    {
      logfile.write("dir.opendir(%s) 失败。\n",starg.localpath); return false;
    }

    while (true)
    {
        if (dir.readdir()==false) break;

        vfromdir.emplace_back(dir.m_filename,dir.m_mtime);
    }

    return true;
}

// 加载starg.okfilename文件中的内容到容器mfromok中。
bool loadokfile()
{
    mfromok.clear();

    cifile ifile;

    // 注意：如果程序是第一次上传，starg.okfilename是不存在的，并不是错误，所以也返回true。
    if ( (ifile.open(starg.okfilename))==false )  return true;

    string strbuffer;

    struct st_fileinfo stfileinfo;

    while (true)
    {
        stfileinfo.clear();

        if (ifile.readline(strbuffer)==false) break;

        getxmlbuffer(strbuffer,"filename",stfileinfo.filename);
        getxmlbuffer(strbuffer,"mtime",stfileinfo.mtime);

        mfromok[stfileinfo.filename]=stfileinfo.mtime;
    }

    //for (auto &aa:mfromok)
    //    logfile.write("filename=%s,mtime=%s\n",aa.first.c_str(),aa.second.c_str());
    
    return true;
}

bool compmap()    
{
    vtook.clear(); 
    vupload.clear();

    // 遍历vfromdir。
    for (auto &aa:vfromdir)
    {
        auto it=mfromok.find(aa.filename);
        if (it !=mfromok.end())
        {
            // 找到了，如果时间也相同，不需要上传，否则需要重新上传。
            if (it->second==aa.mtime) vtook.push_back(aa); 
            else vupload.push_back(aa);
        }
        else
        {  // 如果没有找到，把记录放入vupload容器。
            vupload.push_back(aa);
        }
    }

    return true;
}

// 把容器vtook中的内容写入starg.okfilename文件，覆盖之前的旧starg.okfilename文件。
bool writetookfile()
{
    cofile ofile;    

    if (ofile.open(starg.okfilename)==false)
    {
      logfile.write("file.open(%s) failed.\n",starg.okfilename); return false;
    }

    for (auto &aa:vtook)
        ofile.writeline("<filename>%s</filename><mtime>%s</mtime>\n",aa.filename.c_str(),aa.mtime.c_str());

    ofile.closeandrename();

    return true;
}

bool appendtookfile(struct st_fileinfo &stfileinfo)
{
    cofile ofile;

    // 以追加的方式打开文件，注意第二个参数一定要填false。
    if (ofile.open(starg.okfilename,false,ios::app)==false)
    {
      logfile.write("file.open(%s) failed.\n",starg.okfilename); return false;
    }

    ofile.writeline("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo.filename.c_str(),stfileinfo.mtime.c_str());

    return true;
}