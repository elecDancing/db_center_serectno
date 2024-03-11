/*
 *  obtmindtodb.cpp，本程序用于把气象观测数据文件入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式。
 *  作者：Prtrick。
*/
//#include "_public.h"  // 开发框架的头文件。
//#include "_ooci.h"    // 操作Oracle的头文件。
#include "idcapp.h"    
using namespace idc;

clogfile logfile;        // 日志文件。
connection conn;    // 数据库连接。
cpactive pactive;     // 进程的心跳。

// 业务处理主函数。
bool _obtmindtodb(const char *pathname,const char *connstr,const char *charset);

void EXIT(int sig);     // 程序退出的信号处理函数。

int main(int argc,char *argv[])
{
    // 帮助文档。
    if (argc!=5)
    {
        printf("\n");
        printf("Using:./obtmindtodb pathname connstr charset logfile\n");

        printf("Example:/project/tools/bin/procctl 10 /project/idc/bin/obtmindtodb /idcdata/surfdata "\
                  "\"idc/idcpwd@snorcl11g_128\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtmindtodb.log\n\n");

        printf("本程序用于把全国气象观测数据文件入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式，数据只插入，不更新。\n");
        printf("pathname 全国气象观测数据文件存放的目录。\n");
        printf("connstr  数据库连接参数：username/password@tnsname\n");
        printf("charset  数据库的字符集。\n");
        printf("logfile  本程序运行的日志文件名。\n");
        printf("程序每10秒运行一次，由procctl调度。\n\n\n");

        return -1;
    }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    // clostioandsignal(true); 
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件。
    if (logfile.open(argv[4])==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[4]); return -1;
    }

    pactive.addpinfo(30,"obtmindtodb");   // 进程心跳。

    // 业务处理主函数。
    _obtmindtodb(argv[1],argv[2],argv[3]);

    return 0;
}

void EXIT(int sig)
{
    logfile.write("程序退出，sig=%d\n\n",sig);

    // 可以不写，在析构函数中会回滚事务和断开与数据库的连接。
    conn.rollback();
    conn.disconnect();   

    exit(0);
}

// 业务处理主函数。
bool _obtmindtodb(const char *pathname,const char *connstr,const char *charset)
{
    // 1）打开存放气象观测数据文件的目录。
    cdir dir;
    if (dir.opendir(pathname,"*.xml,*.csv")==false)
    {
        logfile.write("dir.opendir(%s) failed.\n",pathname); return false;
    }

    CZHOBTMIND ZHOBTMIND(conn,logfile);  // 操作气象观测数据表的对象。

    // 2）用循环读取目录中的每个文件。
    while (true)
    {
        // 读取一个气象观测数据文件（只处理*.xml和*.csv）。
        if (dir.readdir()==false) break;

        // 如果有文件需要处理，判断与数据库的连接状态，如果是未连接，就连上数据库。
        if (conn.isopen()==false)
        {
            if (conn.connecttodb(connstr,charset)!=0)
            {
                logfile.write("connect database(%s) failed.\n%s\n",connstr,conn.message()); return false;
            }
    
            logfile.write("connect database(%s) ok.\n",connstr);
        }

        // 打开文件。
        cifile ifile;
        if (ifile.open(dir.m_ffilename)==false)
        {
            logfile.write("file.open(%s) failed.\n",dir.m_ffilename.c_str()); return false;
        }

        int  totalcount=0;     // 文件的总记录数。
        int  insertcount=0;   // 成功插入记录数。
        ctimer timer;            // 计时器，记录每个数据文件的处理耗时。
        bool bisxml=matchstr(dir.m_ffilename,"*.xml");  // 文件格式，true-xml；false-csv。

        string strbuffer;      // 存放从文件中读取的一行数据。

        // 如果是csv文件，扔掉第一行。
        if (bisxml==false)  ifile.readline(strbuffer);

        // 读取文件中的每一行，插入到数据库的表中。
        while(true)
        {
            // 从文件中读取一行。
            if (bisxml==true)
            {
                if (ifile.readline(strbuffer,"<endl/>")==false) break;     // xml文件的行结束标志是<endl/>
            }
            else
            {
                if (ifile.readline(strbuffer)==false) break;                      // csv文件没有行结束标志。 
            }

            totalcount++;       // 文件的总记录数加1。

            // 解析行的内容（*.xml和*.csv的方法不同），把数据存放在结构体中。
            ZHOBTMIND.splitbuffer(strbuffer,bisxml);

            // 把解析后的数据入库（插入到数据库的表中）。
            if (ZHOBTMIND.inserttable()==true) insertcount++;       // 成功插入的记录数加1。
        }

        // 关闭并删除已处理的文件，提交事务。
        ifile.closeandremove();
        conn.commit();
        logfile.write("已处理文件%s（totalcount=%d,insertcount=%d），耗时%.2f秒。\n",\
                              dir.m_ffilename.c_str(),totalcount,insertcount,timer.elapsed());
        pactive.uptatime();   // 进程心跳。
    }

    return true;
}
