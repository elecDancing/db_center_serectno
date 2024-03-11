/*
 *  obtcodetodb.cpp，本程序用于把全国气象站点参数文件中的数据入库到数据库T_ZHOBTCODE表中。
 *  作者：Prtrick。
*/
#include "_public.h"  // 开发框架的头文件。
#include "_ooci.h"    // 操作Oracle的头文件。
using namespace idc;

clogfile logfile;        // 日志文件。
connection conn;    // 数据库连接。
cpactive pactive;     // 进程的心跳。

// 全国气象站点参数结构体。
struct st_stcode
{
    char provname[31];  // 省
    char obtid[11];         // 站号
    char cityname[31];   // 站名
    char lat[11];             // 纬度
    char lon[11];            // 经度
    char height[11];       // 海拔高度
} stcode;
list<struct st_stcode> stcodelist;              // 存放全国气象站点参数的容器。
bool loadstcode(const string &inifile);     // 把站点参数文件中加载到stcodelist容器中。

void EXIT(int sig);     // 程序退出的信号处理函数。

int main(int argc,char *argv[])
{
    // 帮助文档。
    if (argc!=5)
    {
        printf("\n");
        printf("Using:./obtcodetodb inifile connstr charset logfile\n");

        printf("Example:/project/tools/bin/procctl 120 /project/idc/bin/obtcodetodb /project/idc/ini/stcode.ini "\
                  "\"idc/idcpwd@snorcl11g_128\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtcodetodb.log\n\n");

        printf("本程序用于把全国气象站点参数数据保存到数据库的T_ZHOBTCODE表中，如果站点不存在则插入，站点已存在则更新。\n");
        printf("inifile 全国气象站点参数文件名（全路径）。\n");
        printf("connstr 数据库连接参数：username/password@tnsname\n");
        printf("charset 数据库的字符集。\n");
        printf("logfile 本程序运行的日志文件名。\n");
        printf("程序每120秒运行一次，由procctl调度。\n\n\n");

        return -1;
    }

    // 1）处理程序退出的信号、打开日志文件。
    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    // closeioandsignal(true); 
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件。
    if (logfile.open(argv[4])==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[4]); return -1;
    }

    // 注意，在调试程序的时候，禁用以下代码，防止被杀。
    pactive.addpinfo(10,"obtcodetodb");   // 进程的心跳，10秒足够。

    // 2）把全国站点参数文件加载到stcodelist容器中。  
    if (loadstcode(argv[1])==false) EXIT(-1);

    // 3）连接数据库。
    if (conn.connecttodb(argv[2],argv[3])!=0)
    {
        logfile.write("connect database(%s) failed.\n%s\n",argv[2],conn.message()); EXIT(-1);
    }

    logfile.write("connect database(%s) ok.\n",argv[2]);

    // 4）准备插入和更新表的SQL语句。
    sqlstatement stmtins(&conn);  // 插入表的sql语句。
    stmtins.prepare("\
        insert into T_ZHOBTCODE(obtid,cityname,provname,lat,lon,height,keyid) \
                                       values(:1,:2,:3,:4*100,:5*100,:6*10,SEQ_ZHOBTCODE.nextval)");
    stmtins.bindin(1,stcode.obtid,5);
    stmtins.bindin(2,stcode.cityname,30);
    stmtins.bindin(3,stcode.provname,30);
    stmtins.bindin(4,stcode.lat,10);
    stmtins.bindin(5,stcode.lon,10);
    stmtins.bindin(6,stcode.height,10);

    sqlstatement stmtupt(&conn);    // 更新表的sql语句。
    stmtupt.prepare("\
       update T_ZHOBTCODE set cityname=:1,provname=:2,lat=:3*100,lon=:4*100,height=:5*10,upttime=sysdate \
         where obtid=:6");
    stmtupt.bindin(1,stcode.cityname,30);
    stmtupt.bindin(2,stcode.provname,30);
    stmtupt.bindin(3,stcode.lat,10);
    stmtupt.bindin(4,stcode.lon,10);
    stmtupt.bindin(5,stcode.height,10);
    stmtupt.bindin(6,stcode.obtid,5);
    // 抄以上代码的时候要小心，经常有人在这里栽跟斗，或者搞错参数的顺序，或者搞错sqlstatement对象的变量名。

    int inscount=0,uptcount=0;   // 插入记录数和更新记录数。
    ctimer timer;                          // 操作数据库消耗的时间。

    // 5）遍历容器，处理每个站点，向表中插入记录，如果记录已存在，则更新表中的记录。
    for (auto &aa:stcodelist)
    {
        // 从容器中取出一条记录到结构体stcode中。
        stcode=aa;  

        // 执行插入的SQL语句。
        if (stmtins.execute()!=0)
        {
            if (stmtins.rc()==1)    // 错误代码为ORA-0001违返唯一约束，表示该站点的记录在表中已存在。
            {
                // 如果记录已存在，执行更新表的SQL语句。
                if (stmtupt.execute()!=0)
                {
                    logfile.write("stmtupt.execute() failed.\n%s\n%s\n",stmtupt.sql(),stmtupt.message()); EXIT(-1);
                }
                else
                    uptcount++;
            }
            else
            {
                // 其它错误，程序退出。
                logfile.write("stmtins.execute() failed.\n%s\n%s\n",stmtins.sql(),stmtins.message()); EXIT(-1);
            }
        }
        else
            inscount++;
    }

    // 把总记录数、插入记录数、更新记录数、消耗时长写日志。
    logfile.write("总记录数=%d，插入=%d，更新=%d，耗时=%.2f秒。\n",stcodelist.size(),inscount,uptcount,timer.elapsed());

    // 6）提交事务。
    conn.commit();

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

// 把站点参数文件中加载到stcodelist容器中。
bool loadstcode(const string &inifile)
{
    cifile ifile;

    // 打开站点参数文件。
    if (ifile.open(inifile)==false)
    {
        logfile.write("ifile.open(%s) failed.\n",inifile); return false;
    }

    string strbuffer;     // 存放从文件中读取的每一行。
    ccmdstr cmdstr;    // 用于拆分字符串。

    while (true)
    {
        // 从站点参数文件中读取一行，如果已读取完，跳出循环。
        if (ifile.readline(strbuffer)==false) break;

        // 把读取到的一行拆分。
        cmdstr.splittocmd(strbuffer,",");

        if (cmdstr.cmdcount()!=6) continue;     // 扔掉无效的行。

        // 把站点参数的每个数据项保存到站点参数结构体中。
        memset(&stcode,0,sizeof(struct st_stcode));
        cmdstr.getvalue(0, stcode.provname,30); // 省
        cmdstr.getvalue(1, stcode.obtid,5);    // 站号
        cmdstr.getvalue(2, stcode.cityname,30);  // 站名
        cmdstr.getvalue(3, stcode.lat,10);      // 纬度
        cmdstr.getvalue(4, stcode.lon,10);      // 经度
        cmdstr.getvalue(5, stcode.height,10);   // 海拔高度

        // 把站点参数结构体放入站点参数容器。
        stcodelist.push_back(stcode);
    }

    //for (auto &aa:stcodelist)
    //    logfile.write("provname=%s,obtid=%s,cityname=%s,lat=%s,lon=%s,height=%s\n",\
    //               aa.provname,aa.obtid,aa.cityname,aa.lat,aa.lon,aa.height);

    return true;
}