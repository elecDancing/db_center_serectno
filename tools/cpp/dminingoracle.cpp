/*
 *  程序名：dminingoracle.cpp，本程序是数据中心的公共功能模块，用于从Oracle数据库源表抽取数据，生成xml文件。
 *  作者：Prtrick。
*/
#include "_public.h"
#include "_ooci.h"
using namespace idc;

// 程序运行参数的结构体。
struct st_arg
{
    char connstr[101];       // 数据库的连接参数。
    char charset[51];         // 数据库的字符集。
    char selectsql[1024];   // 从数据源数据库抽取数据的SQL语句。
    char fieldstr[501];        // 抽取数据的SQL语句输出结果集字段名，字段名之间用逗号分隔。
    char fieldlen[501];       // 抽取数据的SQL语句输出结果集字段的长度，用逗号分隔。
    char bfilename[31];     // 输出xml文件的前缀。
    char efilename[31];     // 输出xml文件的后缀。
    char outpath[256];      // 输出xml文件存放的目录。
    int    maxcount;           // 输出xml文件最大记录数，0表示无限制。xml文件将用于入库，如果文件太大，数据库会产生大事务。
    char starttime[52];      // 程序运行的时间区间
    char incfield[31];         // 递增字段名。
    char incfilename[256]; // 已抽取数据的递增字段最大值存放的文件。
    char connstr1[101];     // 已抽取数据的递增字段最大值存放的数据库的连接参数。
    int    timeout;              // 进程心跳的超时时间。
    char pname[51];          // 进程名，建议用"dminingoracle_后缀"的方式。
} starg;

// 把xml解析到参数starg结构中。
bool _xmltoarg(const char *strxmlbuffer);

ccmdstr fieldname;         // 结果集字段名数组。
ccmdstr fieldlen;             // 结果集字段长度数组。

connection conn;            // 数据源数据库。
clogfile logfile;

// 程序退出和信号2、15的处理函数。
void EXIT(int sig);

void _help();

// 判断当前时间是否在程序运行的时间区间内。
bool instarttime();

// 数据抽取的主函数。
bool _dminingoracle();

// 1）从文件或数据库中获取上次已抽取数据的增量字段的最大值；（如果是第一次执行抽取任务，增量字段的最大值为0）
// 2）绑定输入变量（已抽取数据的增量字段的最大值）；
// 3）获取结果集的时候，要把增量字段的最大值保存在全局变量imaxincvalue中；
// 4）抽取完数据之后，把i全局变量imaxincvalue中的增量字段的最大值保存在文件或数据库中。
long imaxincvalue;         // 递增字段的最大值。
int  incfieldpos=-1;        // 递增字段在结果集数组中的位置。
bool readincfield();        // 从数据库表中或starg.incfilename文件中加载上次已抽取数据的最大值。
bool writeincfield();       // 把已抽取数据的最大值写入数据库表或starg.incfilename文件。

cpactive pactive;           // 进程心跳。

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
    if (_xmltoarg(argv[2])==false) EXIT(-1);

    // 判断当前时间是否在程序运行的时间区间内。
    if (instarttime()==false) return 0;

    pactive.addpinfo(starg.timeout,starg.pname);  // 把进程的心跳信息写入共享内存。

    // 连接数据源的数据库。
    if (conn.connecttodb(starg.connstr,starg.charset)!=0)
    {
        logfile.write("connect database(%s) failed.\n%s\n",starg.connstr,conn.message()); EXIT(-1);
    }
    logfile.write("connect database(%s) ok.\n",starg.connstr);

    // 从数据库表中或starg.incfilename文件中获取已抽取数据的最大值。
    if (readincfield()==false) EXIT(-1);

    _dminingoracle();     // 数据抽取的主函数。

    return 0;
}

// 数据抽取的主函数。
bool _dminingoracle()
{
    // 1）准备抽取数据的SQL语句。
    sqlstatement stmt(&conn);
    stmt.prepare(starg.selectsql);

    // 2）绑定结果集的变量。
    string strfieldvalue[fieldname.size()];
    for (int ii=1;ii<=fieldname.size();ii++)
    {
        stmt.bindout(ii,strfieldvalue[ii-1],stoi(fieldlen[ii-1]));
    }

    // 如果是增量抽取，绑定输入参数（已抽取数据的递增字段的最大值）。
    if (strlen(starg.incfield)!=0) stmt.bindin(1,imaxincvalue);

    // 3）执行抽取数据的SQL语句。
    if (stmt.execute()!=0)
    {
        logfile.write("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); return false;
    }

    pactive.uptatime();       // 更新进程的心跳。

    string strxmlfilename;  // 输出的xml文件名，例如：ZHOBTCODE_20230419162835_togxpt_1.xml
    int iseq=1;                    // 输出xml文件的序号。
    cofile ofile;                   // 用于向xml文件中写入数据。

    // 4）获取结果集中的记录，写入xml文件。
    while (true)
    {
        if (stmt.next()!=0) break;         // 从结果集中获取一行记录。

        if (ofile.isopen()==false)
        {
            // 如果xml文件是未打开状态，打开xml文件。
            sformat(strxmlfilename,"%s/%s_%s_%s_%d.xml",\
                  starg.outpath,starg.bfilename,ltime1("yyyymmddhh24miss").c_str(),starg.efilename,iseq++);
            if (ofile.open(strxmlfilename)==false)
            {
                logfile.write("ofile.open(%s) failed.\n",strxmlfilename.c_str()); return false;
            }
            ofile.writeline("<data>\n");          // 写入数据集开始的标签。
        }  

        // 把结果集中的每个字段的值写入xml文件。
        for (int ii=1;ii<=fieldname.size();ii++)
            ofile.writeline("<%s>%s</%s>",fieldname[ii-1].c_str(),strfieldvalue[ii-1].c_str(),fieldname[ii-1].c_str());

        ofile.writeline("<endl/>\n");    // 写入每行的结束标志。

        // 如果记录数达到starg.maxcount行就关闭当前文件。200 838 200 400 600
        if ( (starg.maxcount>0) && (stmt.rpc()%starg.maxcount==0) )
        {
            ofile.writeline("</data>\n");   // 写入文件的结束标志。

            if (ofile.closeandrename()==false)  // 关闭文件，把临时文件名改名为正式的文件名。
            {
                logfile.write("ofile.closeandrename(%s) failed.\n",strxmlfilename.c_str()); return false;
            }

            logfile.write("生成文件%s(%d)。\n",strxmlfilename.c_str(),starg.maxcount);     

            pactive.uptatime();    // 更新进程的心跳。 
        }

        // 更新递增字段的最大值。
        if ( (strlen(starg.incfield)!=0) && (imaxincvalue<stol(strfieldvalue[incfieldpos])) )
           imaxincvalue=stol(strfieldvalue[incfieldpos]);
    }

    // 5）如果maxcount==0或者向xml文件中写入的记录数不足maxcount，关闭文件。
    if (ofile.isopen()==true)
    {
        ofile.writeline("</data>\n");        // 写入数据集结束的标签。
        if (ofile.closeandrename()==false)
        {
            logfile.write("ofile.closeandrename(%s) failed.\n",strxmlfilename.c_str()); return false;
        }

        if (starg.maxcount==0)
            logfile.write("生成文件%s(%d)。\n",strxmlfilename.c_str(),stmt.rpc());
        else
            logfile.write("生成文件%s(%d)。\n",strxmlfilename.c_str(),stmt.rpc()%starg.maxcount);
    }

    // 把已抽取数据的最大值写入数据库表或starg.incfilename文件。
    if (stmt.rpc()>0) writeincfield(); 

    return true;
}

// 判断当前时间是否在程序运行的时间区间内。
bool instarttime()
{
    // 程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。
    if (strlen(starg.starttime)!=0)
    {
        string strhh24=ltime1("hh24");  // 获取当前时间的小时，如果当前时间是2023-01-08 12:35:40，将得到12。
        if (strstr(starg.starttime,strhh24.c_str())==0) return false;
    }       // 闲时：12-14时和00-06时。

    return true;
}

void EXIT(int sig)
{
    logfile.write("程序退出，sig=%d\n\n",sig);

    exit(0);
}

// 本程序的帮助文档。
void _help()
{
    printf("Using:/project/tools/bin/dminingoracle logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 3600 /project/tools/bin/dminingoracle /log/idc/dminingoracle_ZHOBTCODE.log "
              "\"<connstr>idc/idcpwd@snorcl11g_128</connstr><charset>Simplified Chinese_China.AL32UTF8</charset>"\
              "<selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE where obtid like '5%%%%'</selectsql>"\
              "<fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>5,30,30,10,10,10</fieldlen>"\
              "<bfilename>ZHOBTCODE</bfilename><efilename>togxpt</efilename><outpath>/idcdata/dmindata</outpath>"\
              "<timeout>30</timeout><pname>dminingoracle_ZHOBTCODE</pname>\"\n\n");
    printf("       /project/tools/bin/procctl   30 /project/tools/bin/dminingoracle /log/idc/dminingoracle_ZHOBTMIND.log "\
              "\"<connstr>idc/idcpwd@snorcl11g_128</connstr><charset>Simplified Chinese_China.AL32UTF8</charset>"\
              "<selectsql>select obtid,to_char(ddatetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and obtid like '5%%%%'</selectsql>"\
              "<fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>5,19,8,8,8,8,8,8,8,15</fieldlen>"\
              "<bfilename>ZHOBTMIND</bfilename><efilename>togxpt</efilename><outpath>/idcdata/dmindata</outpath>"\
              "<starttime></starttime><incfield>keyid</incfield>"\
              "<incfilename>/idcdata/dmining/dminingoracle_ZHOBTMIND_togxpt.keyid</incfilename>"\
              "<timeout>30</timeout><pname>dminingoracle_ZHOBTMIND_togxpt</pname>"\
              "<maxcount>1000</maxcount><connstr1>scott/tiger@snorcl11g_128</connstr1>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于从Oracle数据库源表抽取数据，生成xml文件。\n");
    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据源数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("selectsql   从数据源数据库抽取数据的SQL语句，如果是增量抽取，一定要用递增字段作为查询条件，如where keyid>:1。\n");
    printf("fieldstr    抽取数据的SQL语句输出结果集的字段名列表，中间用逗号分隔，将作为xml文件的字段名。\n");
    printf("fieldlen    抽取数据的SQL语句输出结果集字段的长度列表，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应。\n");
    printf("outpath     输出xml文件存放的目录。\n");
    printf("bfilename   输出xml文件的前缀。\n");
    printf("efilename   输出xml文件的后缀。\n"); 
    printf("maxcount    输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"\
         "如果starttime为空，表示不启用，只要本程序启动，就会执行数据抽取任务，为了减少数据源数据库压力"\
         "抽取数据的时候，如果对时效性没有要求，一般在数据源数据库空闲的时候时进行。\n");
    printf("incfield    递增字段名，它必须是fieldstr中的字段名，并且只能是整型，一般为自增字段。"\
          "如果incfield为空，表示不采用增量抽取的方案。");
    printf("incfilename 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
    printf("connstr1    已抽取数据的递增字段最大值存放的数据库的连接参数。connstr1和incfilename二选一，connstr1优先。");
    printf("timeout     本程序的超时时间，单位：秒。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}

// 把xml解析到参数starg结构中。
bool _xmltoarg(const char *strxmlbuffer)
{
    memset(&starg,0,sizeof(struct st_arg));

    getxmlbuffer(strxmlbuffer,"connstr",starg.connstr,100);       // 数据源数据库的连接参数。
    if (strlen(starg.connstr)==0) { logfile.write("connstr is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"charset",starg.charset,50);         // 数据库的字符集。
    if (strlen(starg.charset)==0) { logfile.write("charset is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"selectsql",starg.selectsql,1000);  // 从数据源数据库抽取数据的SQL语句。
    if (strlen(starg.selectsql)==0) { logfile.write("selectsql is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"fieldstr",starg.fieldstr,500);          // 结果集字段名列表。
    if (strlen(starg.fieldstr)==0) { logfile.write("fieldstr is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"fieldlen",starg.fieldlen,500);         // 结果集字段长度列表。
    if (strlen(starg.fieldlen)==0) { logfile.write("fieldlen is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"bfilename",starg.bfilename,30);   // 输出xml文件存放的目录。
    if (strlen(starg.bfilename)==0) { logfile.write("bfilename is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"efilename",starg.efilename,30);    // 输出xml文件的前缀。
    if (strlen(starg.efilename)==0) { logfile.write("efilename is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"outpath",starg.outpath,255);        // 输出xml文件的后缀。
    if (strlen(starg.outpath)==0) { logfile.write("outpath is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"maxcount",starg.maxcount);       // 输出xml文件的最大记录数，可选参数。

    getxmlbuffer(strxmlbuffer,"starttime",starg.starttime,50);     // 程序运行的时间区间，可选参数。

    getxmlbuffer(strxmlbuffer,"incfield",starg.incfield,30);          // 递增字段名，可选参数。

    getxmlbuffer(strxmlbuffer,"incfilename",starg.incfilename,255);  // 已抽取数据的递增字段最大值存放的文件，可选参数。

    getxmlbuffer(strxmlbuffer,"connstr1",starg.connstr1,100);          // 已抽取数据的递增字段最大值存放的数据库的连接参数，可选参数。

    getxmlbuffer(strxmlbuffer,"timeout",starg.timeout);       // 进程心跳的超时时间。
    if (starg.timeout==0) { logfile.write("timeout is null.\n");  return false; }

    getxmlbuffer(strxmlbuffer,"pname",starg.pname,50);     // 进程名。
    if (strlen(starg.pname)==0) { logfile.write("pname is null.\n");  return false; }

    // 拆分starg.fieldstr到fieldname中。
    fieldname.splittocmd(starg.fieldstr,",");

    // 拆分starg.fieldlen到fieldlen中。
    fieldlen.splittocmd(starg.fieldlen,",");

    // 判断fieldname和fieldlen两个数组的大小是否相同。
    if (fieldlen.size()!=fieldname.size())
    {
        logfile.write("fieldstr和fieldlen的元素个数不一致。\n"); return false;
    }

    // 如果是增量抽取，incfilename和connstr1必二选一。
    if (strlen(starg.incfield)>0)
    {
        if ( (strlen(starg.incfilename)==0) && (strlen(starg.connstr1)==0) )
        {
            logfile.write("如果是增量抽取，incfilename和connstr1必二选一，不能都为空。\n"); return false;
        }
    }

    return true;
}

// 从数据库表中或starg.incfilename文件中加载上次已抽取数据的最大值。
bool readincfield()
{
    imaxincvalue=0;    // 初始化递增字段的最大值。

    // 如果starg.incfield参数为空，表示不是增量抽取。
    if (strlen(starg.incfield)==0) return true;

    // 查找递增字段在结果集中的位置。
    for (int ii=0;ii<fieldname.size();ii++)
        if (fieldname[ii]==starg.incfield) { incfieldpos=ii; break; }

    if (incfieldpos==-1)
    {
        logfile.write("递增字段名%s不在列表%s中。\n",starg.incfield,starg.fieldstr); return false;
    }

    if (strlen(starg.connstr1)!=0)
    {
        // 从数据库表中加载递增字段的最大值。
        connection conn1;
        if (conn1.connecttodb(starg.connstr1,starg.charset)!=0)
        {
            logfile.write("connect database(%s) failed.\n%s\n",starg.connstr1,conn1.message()); return false;
        }
        sqlstatement stmt(&conn1);
        stmt.prepare("select maxincvalue from T_MAXINCVALUE where pname=:1");
        stmt.bindin(1,starg.pname);
        stmt.bindout(1,imaxincvalue);
        stmt.execute();
        stmt.next();
    }
    else
    {
        // 从文件中加载递增字段的最大值。
        cifile ifile; 

        // 如果打开starg.incfilename文件失败，表示是第一次运行程序，也不必返回失败。
        // 也可能是文件丢了，那也没办法，只能重新抽取。
        if (ifile.open(starg.incfilename)==false) return true;

        // 从文件中读取已抽取数据的最大值。
        string strtemp;
        ifile.readline(strtemp);
        imaxincvalue=stol(strtemp);
    }

    logfile.write("上次已抽取数据的位置（%s=%ld）。\n",starg.incfield,imaxincvalue);

    return true;
}

// 把已抽取数据的最大值写入数据库表或starg.incfilename文件。
bool writeincfield()
{
    // 如果starg.incfield参数为空，表示不是增量抽取。
    if (strlen(starg.incfield)==0) return true;

     if (strlen(starg.connstr1)!=0)
    {
        // 把递增字段的最大值写入数据库的表。
        connection conn1;
        if (conn1.connecttodb(starg.connstr1,starg.charset)!=0)
        {
            logfile.write("connect database(%s) failed.\n%s\n",starg.connstr1,conn1.message()); return false;
        }
        sqlstatement stmt(&conn1);
        stmt.prepare("update T_MAXINCVALUE set maxincvalue=:1 where pname=:2");
        stmt.bindin(1,imaxincvalue);
        stmt.bindin(2,starg.pname);
        if (stmt.execute()!=0)
        {
            if (stmt.rc()==942)  // 如果表不存在，stmt.execute()将返回ORA-00942的错误。
            {
                // 如果表不存在，就创建表，然后插入记录。
                conn1.execute("create table T_MAXINCVALUE(pname varchar2(50),maxincvalue number(15),primary key(pname))");
                conn1.execute("insert into T_MAXINCVALUE values('%s',%ld)",starg.pname,imaxincvalue);
                conn1.commit();
                return true;
            }
            else
            {
                logfile.write("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); return false;
            }
        }
        else
        {
            if (stmt.rpc()==0)
            {
			    // 如果记录不存在，就插入新记录。
                conn1.execute("insert into T_MAXINCVALUE values('%s',%ld)",starg.pname,imaxincvalue);
            }
            conn1.commit();
        }
    }
    else
    {
        // 把递增字段的最大值写入文件。
        cofile ofile;

        if (ofile.open(starg.incfilename,false)==false) 
        {
            logfile.write("ofile.open(%s) failed.\n",starg.incfilename); return false;
        }

        // 把已抽取数据的最大id写入文件。
        ofile.writeline("%ld",imaxincvalue);
    }

    return true;
}
