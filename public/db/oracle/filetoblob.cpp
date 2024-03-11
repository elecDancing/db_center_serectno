/*
 *  程序名：filetoblob.cpp，此程序演示开发框架操作Oracle数据库（把二进制文件存入数据库的BLOB字段中）。
 *  作者：Prtrick。
*/
#include "_ooci.h"   // 开发框架操作Oracle的头文件。
using namespace idc;

int main(int argc,char *argv[])
{
    connection conn; // 创建数据库连接类的对象。

    // 登录数据库，返回值：0-成功，其它-失败。
    // 失败代码在conn.m_cda.rc中，失败描述在conn.m_cda.message中。
    if (conn.connecttodb("scott/tiger@snorcl11g_128","Simplified Chinese_China.AL32UTF8") != 0)
    {
        printf("connect database failed.\n%s\n",conn.message()); return -1;
    }

    printf("connect database ok.\n");

    sqlstatement stmt(&conn); 
    stmt.prepare("insert into girls(id,name,pic) values(1,'冰冰',empty_blob())");  // 注意：不可用null代替empty_blob()。
    if (stmt.execute()!=0)
    {
        printf("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); return -1;
    }

    // 使用游标从girls表中提取记录的pic字段
    stmt.prepare("select pic from girls where id=1 for update");
    stmt.bindblob();

    // 执行SQL语句，一定要判断返回值，0-成功，其它-失败。
    if (stmt.execute() != 0)
    {
        printf("stmt.execute() failed.\n%s\n%s\n",stmt.sql(),stmt.message()); return -1;
    }

    // 获取一条记录，一定要判断返回值，0-成功，1403-无记录，其它-失败。
    if (stmt.next() != 0) return 0;

    // 把磁盘文件pic_in.jpeg的内容写入BLOB字段，一定要判断返回值，0-成功，其它-失败。
    if (stmt.filetolob("/project/public/db/oracle/pic_in.jpeg") != 0)
    {
        printf("stmt.filetolob() failed.\n%s\n",stmt.message()); return -1;
    }

    printf("二进制文件已存入数据库的BLOB字段中。\n");

    conn.commit();

    return 0;
}