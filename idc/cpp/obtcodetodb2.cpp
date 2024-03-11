/*
 *  obtcodetodb.cpp，本程序用于把全国气象站点参数文件中的数据入库到数据库T_ZHOBTCODE表中。
 *  作者：Prtrick。
*/
#include "_public.h"  // 开发框架的头文件。
#include "_ooci.h"    // 操作Oracle的头文件。
using namespace idc;

clogfile logfile;        // 日志文件。
connection conn;    // 数据库连接。

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

        printf("Example:/project/tools/bin/procctl 120 /project/idc/bin/obtcodetodb1 /tmp/ZHOBTCODE_20230506183336_toidc_1.xml "\
                  "\"idc/idcpwd@snorcl11g_128\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtcodetodb1.log\n\n");

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

    // 打开日志文件。
    if (logfile.open(argv[4])==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[4]); return -1;
    }

    // 3）连接数据库。
    if (conn.connecttodb(argv[2],argv[3])!=0)
    {
        logfile.write("connect database(%s) failed.\n%s\n",argv[2],conn.message()); EXIT(-1);
    }

    //vector<string> colvalue;
    string colvalue[10];
    //colvalue.resize(10);

    sqlstatement stmt;
    stmt.connect(&conn);
    stmt.prepare("\
        insert into T_ZHOBTCODE1(obtid,cityname,provname,lat,lon,height,keyid) \
                                       values(:1,:2,:3,:4,:5,:6,SEQ_ZHOBTCODE1.nextval)");
    stmt.bindin(1,colvalue[0],5);
    stmt.bindin(2,colvalue[1],30);
    stmt.bindin(3,colvalue[2],30);
    stmt.bindin(4,colvalue[3],20);
    stmt.bindin(5,colvalue[4],20);
    stmt.bindin(6,colvalue[5],20);

    cifile ifile;
    ifile.open(argv[1]);
    // <obtid>57399</obtid><cityname>麻城</cityname><provname>湖北</provname><lat>3108</lat><lon>11457</lon><height>743</height><endl/>
    string buffer;
    while (true)
    {
        if (ifile.readline(buffer,"<endl/>")==false) break;

        getxmlbuffer(buffer,"obtid",colvalue[0],5);
        getxmlbuffer(buffer,"cityname",colvalue[1],30);
        getxmlbuffer(buffer,"provname",colvalue[2],30);
        getxmlbuffer(buffer,"lat",colvalue[3],20);
        getxmlbuffer(buffer,"lon",colvalue[4],20);
        getxmlbuffer(buffer,"height",colvalue[5],20);

        logfile.write("obtid=%s,cityname=%s,provname=%s,lat=%s,lon=%s,height=%s\n",
                  colvalue[0].c_str(),colvalue[1].c_str(),colvalue[2].c_str(),colvalue[3].c_str(),colvalue[4].c_str(),colvalue[5].c_str());

        if (stmt.execute()!=0)
        {
            logfile.write("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); EXIT(-1);
        }
    }

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