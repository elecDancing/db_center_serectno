/*
 *  程序名：migratetable.cpp，本程序是共享平台的公共功能模块，用于迁移表中的数据。
 *  作者：Prtrick。
*/
#include "_tools.h"
using namespace idc;

struct st_arg
{
    char connstr[101];     // 数据库的连接参数。
    char tname[31];        // 待迁移的表名。
    char totname[31];     // 目的表名。
    char keycol[31];        // 待迁移的表的唯一键字段名。
    char where[1001];    // 待迁移的数据需要满足的条件。
    int    maxcount;        // 执行一次SQL删除的记录数。
    char starttime[31];   // 程序运行的时间区间。
    int  timeout;             // 本程序运行时的超时时间。
    char pname[51];      // 本程序运行时的程序名。
} starg;

// 显示程序的帮助
void _help();

// 把xml解析到参数starg结构中
bool _xmltoarg(const char *strxmlbuffer);

clogfile logfile;

// 判断当前时间是否在程序运行的时间区间内。
bool instarttime();

connection conn; 

// 业务处理主函数。
bool _migratetable();
 
void EXIT(int sig);

cpactive pactive;

int main(int argc,char *argv[])
{
    if (argc!=3) { _help(); return -1; }

    // 关闭全部的信号和输入输出
    // 处理程序退出的信号
    closeioandsignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    if (logfile.open(argv[1])==false)
    {
        printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
    }

    // 把xml解析到参数starg结构中
    if (_xmltoarg(argv[2])==false) return -1;

    // 判断当前时间是否在程序运行的时间区间内。
    if (instarttime()==false) return 0;

    // 注意，在调试程序的时候，可以启用类似以下的代码，防止超时。
    // pactive.addpinfo(starg.timeout*10000,starg.pname);
    pactive.addpinfo(starg.timeout,starg.pname);

    if (conn.connecttodb(starg.connstr,"Simplified Chinese_China.AL32UTF8") != 0)  // 字符集随便填。
    {
        logfile.write("connect database(%s) failed.\n%s\n",starg.connstr,conn.message()); EXIT(-1);
    }

    // 业务处理主函数。
    _migratetable();
}

// 显示程序的帮助
void _help()
{
    printf("Using:/project/tools/bin/migratetable logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 3600 /project/tools/bin/migratetable /log/idc/migratetable_ZHOBTMIND1.log "\
                         "\"<connstr>idc/idcpwd@snorcl11g_128</connstr><tname>T_ZHOBTMIND1</tname>"\
                         "<totname>T_ZHOBTMIND1_HIS</totname><keycol>rowid</keycol><where>where ddatetime<sysdate-0.03</where>"\
                         "<maxcount>10</maxcount><starttime>22,23,00,01,02,03,04,05,06,13</starttime>"\
                         "<timeout>120</timeout><pname>migratetable_ZHOBTMIND1</pname>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于迁移表中的数据。\n");

    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("tname       待迁移数据表的表名，例如T_ZHOBTMIND1。\n");
    printf("totname     目的表名，例如T_ZHOBTMIND1_HIS。\n");
    printf("keycol      待迁移数据表的唯一键字段名，可以用记录编号，如keyid，建议用rowid，效率最高。\n");
    printf("where       待迁移的数据需要满足的条件，即SQL语句中的where部分。\n");
    printf("maxcount    执行一次SQL语句删除的记录数，建议在100-500之间。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序运行时，踏中02时和13时则运行，其它时间不运行。"\
                                "如果starttime为空，本参数将失效，只要本程序启动就会执行数据迁移，"\
                                "为了减少对数据库的压力，数据迁移一般在业务最闲的时候时进行。\n");
    printf("timeout     本程序的超时时间，单位：秒，建议设置120以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

// 把xml解析到参数starg结构中
bool _xmltoarg(const char *strxmlbuffer)
{
    memset(&starg,0,sizeof(struct st_arg));

    getxmlbuffer(strxmlbuffer,"connstr",starg.connstr,100);
    if (strlen(starg.connstr)==0) { logfile.write("connstr is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"tname",starg.tname,30);
    if (strlen(starg.tname)==0) { logfile.write("tname is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"totname",starg.totname,30);
    if (strlen(starg.totname)==0) { logfile.write("totname is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"keycol",starg.keycol,30);
    if (strlen(starg.keycol)==0) { logfile.write("keycol is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"where",starg.where,1000);
    if (strlen(starg.where)==0) { logfile.write("where is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"starttime",starg.starttime,30);

    getxmlbuffer(strxmlbuffer,"maxcount",starg.maxcount);
    if (starg.maxcount==0) { logfile.write("maxcount is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"timeout",starg.timeout);
    if (starg.timeout==0) { logfile.write("timeout is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"pname",starg.pname,50);
    if (strlen(starg.pname)==0) { logfile.write("pname is null.\n"); return false; }

    return true;
}

void EXIT(int sig)
{
    logfile.write("程序退出，sig=%d\n\n",sig);

    conn.disconnect();

    exit(0);
}

// 业务处理主函数。
bool _migratetable()
{
    ctimer timer;

    char tmpvalue[21];      // 存放待删除记录的唯一键的值。

    // 1）准备从表中提取数据的SQL语句。
	// select rowid from T_ZHOBTMIND1 where ddatetime<sysdate-1
    sqlstatement stmtsel(&conn);
    stmtsel.prepare("select %s from %s %s",starg.keycol,starg.tname,starg.where);
    stmtsel.bindout(1,tmpvalue,20);

	// 2）准备从表中删除数据的SQL语句，绑定输入参数。
	// delete from T_ZHOBTMIND1 where rowid in (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10);
    string strsql=sformat("delete from %s where %s in (",starg.tname,starg.keycol);    
    for (int ii=0;ii<starg.maxcount;ii++)
    {
        strsql=strsql+sformat(":%lu,",ii+1);
    }
    deleterchr(strsql,',');          // 最后一个逗号是多余的。
    strsql=strsql+")";

    char keyvalues[starg.maxcount][21];   // 存放唯一键字段的值的数组。

    sqlstatement stmtdel(&conn);
    stmtdel.prepare(strsql);                       // 准备删除数据的SQL语句。
    for (int ii=0;ii<starg.maxcount;ii++)
        stmtdel.bindin(ii+1,keyvalues[ii],20);

    // 3）准备插入目的表的SQL语句，绑定输入参数。
    // insert into T_ZHOBTMIND1_HIS(全部的字段) select 全部的字段 from T_ZHOBTMIND1 where rwoid in (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10);
    ctcols tcols;
    tcols.allcols(conn,starg.tname);       // 获取表全部的字段列表。
    strsql=sformat("insert into %s select %s from %s where %s in (",starg.totname,tcols.m_allcols.c_str(),starg.tname,starg.keycol);    
    for (int ii=0;ii<starg.maxcount;ii++)
    {
        strsql=strsql+sformat(":%lu,",ii+1);
    }
    deleterchr(strsql,',');          // 最后一个逗号是多余的。
    strsql=strsql+")";

    sqlstatement stmtins(&conn);
    stmtins.prepare(strsql);                       // 准备插入数据的SQL语句。
    for (int ii=0;ii<starg.maxcount;ii++)
        stmtins.bindin(ii+1,keyvalues[ii],20);

    if (stmtsel.execute()!=0)                      // 执行提取数据的SQL语句。
    {
        logfile.write("stmtsel.execute() failed.\n%s\n%s\n",stmtsel.sql(),stmtsel.message()); return false;
    }

    int ccount=0;                 // keyvalues数组中有效元素的个数。
    memset(keyvalues,0,sizeof(keyvalues));

	// 4）处理结果集。
	while (true) 
	{
		// a）从结果集中获取一行记录，放在临时的数组中。
        memset(tmpvalue,0,sizeof(tmpvalue));
        if (stmtsel.next()!=0) break;
        strcpy(keyvalues[ccount],tmpvalue);
        ccount++;

        // b）如果数组中的记录数达到了starg.maxcount，执行一次删除数据的SQL语句。
        if (ccount==starg.maxcount)
        {
            if (stmtins.execute()!=0)       // 执行插入数据的SQL语句。
            {
                logfile.write("stmtins.execute() failed.\n%s\n%s\n",stmtins.sql(),stmtins.message()); return false;
            }

            if (stmtdel.execute()!=0)       // 执行从表中删除数据的SQL语句。
            {
                logfile.write("stmtdel.execute() failed.\n%s\n%s\n",stmtdel.sql(),stmtdel.message()); return false;
            }

            conn.commit();                     // 提交事务。
            ccount=0;
            memset(keyvalues,0,sizeof(keyvalues));

            pactive.uptatime();               // 进程心跳。
        }
	}

    // 4）如果临时数组中还有记录，再执行一次删除数据的SQL语句。
    if (ccount>0)
    {
        if (stmtins.execute()!=0)
        {
            logfile.write("stmtins.execute() failed.\n%s\n%s\n",stmtins.sql(),stmtins.message()); return false;
        }

        if (stmtdel.execute()!=0)
        {
            logfile.write("stmtdel.execute() failed.\n%s\n%s\n",stmtdel.sql(),stmtdel.message()); return false;
        }

        conn.commit();
    }

    if (stmtsel.rpc()>0) logfile.write("migrate %s to %s %d rows in %.02fsec.\n",starg.tname,starg.totname,stmtsel.rpc(),timer.elapsed());

    return true;
}

// 判断当前时间是否在程序运行的时间区间内。
bool instarttime()
{
    // 程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。
    if (strlen(starg.starttime)!=0)
    {
        if (strstr(starg.starttime,ltime1("hh24").c_str())==0) return false;
    }

    return true;
}