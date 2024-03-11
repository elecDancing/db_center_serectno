/****************************************************************************************/
/*   程序名：idcapp.cpp，此程序是共享平台项目公用函数和类的定义文件。                    */
/*   作者：Prtrick                                                                                                             */
/****************************************************************************************/

#include "idcapp.h"

//  把从文件读到的一行数据拆分到m_zhobtmind结构体中。
bool CZHOBTMIND::splitbuffer(const string &strbuffer,const bool bisxml)
{
    memset(&m_zhobtmind,0,sizeof(struct st_zhobtmind));

    // 解析行的内容（*.xml和*.csv的方法不同），把数据存放在结构体中。
    if (bisxml==true)
    {
        getxmlbuffer(strbuffer,"obtid",m_zhobtmind.obtid,5);
        getxmlbuffer(strbuffer,"ddatetime",m_zhobtmind.ddatetime,14); 
        char tmp[11];
        getxmlbuffer(strbuffer,"t",tmp,10);     if (strlen(tmp)>0) snprintf(m_zhobtmind.t,10,"%d",(int)(atof(tmp)*10));
        getxmlbuffer(strbuffer,"p",tmp,10);    if (strlen(tmp)>0) snprintf(m_zhobtmind.p,10,"%d",(int)(atof(tmp)*10));
        getxmlbuffer(strbuffer,"u",m_zhobtmind.u,10);
        getxmlbuffer(strbuffer,"wd",m_zhobtmind.wd,10);
        getxmlbuffer(strbuffer,"wf",tmp,10);  if (strlen(tmp)>0) snprintf(m_zhobtmind.wf,10,"%d",(int)(atof(tmp)*10));
        getxmlbuffer(strbuffer,"r",tmp,10);     if (strlen(tmp)>0) snprintf(m_zhobtmind.r,10,"%d",(int)(atof(tmp)*10));
        getxmlbuffer(strbuffer,"vis",tmp,10);  if (strlen(tmp)>0) snprintf(m_zhobtmind.vis,10,"%d",(int)(atof(tmp)*10));
    }
    else
    {
        ccmdstr cmdstr;
        cmdstr.splittocmd(strbuffer,",");
        cmdstr.getvalue(0,m_zhobtmind.obtid,5);
        cmdstr.getvalue(1,m_zhobtmind.ddatetime,14);
        char tmp[11];
        cmdstr.getvalue(2,tmp,10); if (strlen(tmp)>0) snprintf(m_zhobtmind.t,10,"%d",(int)(atof(tmp)*10));
        cmdstr.getvalue(3,tmp,10); if (strlen(tmp)>0) snprintf(m_zhobtmind.p,10,"%d",(int)(atof(tmp)*10));
        cmdstr.getvalue(4,m_zhobtmind.u,10);
        cmdstr.getvalue(5,m_zhobtmind.wd,10);
        cmdstr.getvalue(6,tmp,10); if (strlen(tmp)>0) snprintf(m_zhobtmind.wf,10,"%d",(int)(atof(tmp)*10));
        cmdstr.getvalue(7,tmp,10); if (strlen(tmp)>0) snprintf(m_zhobtmind.r,10,"%d",(int)(atof(tmp)*10));
        cmdstr.getvalue(8,tmp,10); if (strlen(tmp)>0) snprintf(m_zhobtmind.vis,10,"%d",(int)(atof(tmp)*10));
    }

    m_buffer=strbuffer;

    return true;
}

// 把m_zhobtmind结构体中的数据插入到T_ZHOBTMIND表中。
bool CZHOBTMIND::inserttable()
{
    if (m_stmt.isopen()==false)
    {
        // 准备操作表的sql语句，绑定输入参数。
        m_stmt.connect(&m_conn);
        m_stmt.prepare("insert into T_ZHOBTMIND(obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid) "\
                                   "values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,SEQ_ZHOBTMIND.nextval)");
        m_stmt.bindin(1,m_zhobtmind.obtid,5);
        m_stmt.bindin(2,m_zhobtmind.ddatetime,14);
        m_stmt.bindin(3,m_zhobtmind.t,10);
        m_stmt.bindin(4,m_zhobtmind.p,10);
        m_stmt.bindin(5,m_zhobtmind.u,10);
        m_stmt.bindin(6,m_zhobtmind.wd,10);
        m_stmt.bindin(7,m_zhobtmind.wf,10);
        m_stmt.bindin(8,m_zhobtmind.r,10);
        m_stmt.bindin(9,m_zhobtmind.vis,10);
    }

    // 把解析后的数据入库（插入到数据库的表中）。
    if (m_stmt.execute()!=0)
    {
        // 失败的原因主要有二：一是记录重复，二是数据内容非法。
        // 如果失败的原因是数据内容非法，记录日志后继续；如果是记录重复，不必记录日志，且继续。
        if (m_stmt.rc()!=1)
        {
            m_logfile.write("strbuffer=%s\n",m_buffer.c_str());
            m_logfile.write("m_stmt.execute() failed.\n%s\n%s\n",m_stmt.sql(),m_stmt.message());
        }

        return false;
    }

    return true;
}