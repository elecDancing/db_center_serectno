#include "_public.h"
using namespace idc;

// 进程心跳信息的结构体。
struct stprocinfo
{
    int      pid=0;                 // 进程id。
    char   pname[51]={0};   // 进程名称，可以为空。
    int      timeout=0;         // 超时时间，单位：秒。
    time_t atime=0;           // 最后一次心跳的时间，用整数表示。
    stprocinfo() = default;   // 有了自定义的构造函数，编译器将不提供默认构造函数，所以启用默认构造函数。
    stprocinfo(const int in_pid,const string & in_pname,const int in_timeout, const time_t in_atime) 
                     : pid(in_pid),timeout(in_timeout),atime(in_atime) 
	{ 
		strncpy(pname,in_pname.c_str(),50); 
	}
};

int  m_shmid=-1;                   // 共享内存的id。
stprocinfo *m_shm=nullptr;  // 指向共享内存的地址空间。
int m_pos=-1;                        // 用于存放当前进程在数组中的下标。

void EXIT(int sig);    // 退出信号处理函数。

int main()
{
    // 处理程序的退出信号。
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);
    
    // 创建/获取共享内存。
    if ( (m_shmid = shmget((key_t)0x5095, 1000*sizeof(struct stprocinfo), 0666|IPC_CREAT)) == -1)
    { 
        printf("创建/获取共享内存(%x)失败。\n",0x5095); return -1; 
    }

    // 将共享内存连接到当前进程的地址空间。
    m_shm=(struct stprocinfo *)shmat(m_shmid, 0, 0);

    // 把共享内存中全部进程的信息显示出来，用于调试。
    for (int ii=0;ii<1000;ii++)
    {
        if ( m_shm[ii].pid!=0 )  // 只显示进程已使用的位置，空位置不显示。
        { 
            printf("ii=%d,pid=%d,pname=%s,timeout=%d,atime=%d\n",
                        ii,m_shm[ii].pid,m_shm[ii].pname,m_shm[ii].timeout,m_shm[ii].atime);
        }
    }

    // 把当前进程的信息填充到结构体中。
    //stprocinfo procinfo;
    //memset(&procinfo,0,sizeof(struct stprocinfo));
    //procinfo.pid=getpid();                           // 当前进程号。
    //strncpy(procinfo.pname,"server1",50);  // 进程名。
    //procinfo.timeout=30;                            // 超时时间。
    //procinfo.atime=time(0);                        // 用当前时间填充最后一次心跳的时间。
    stprocinfo procinfo(getpid(),"server1",30,time(0));

    csemp semlock;                             // 用于给共享内存加锁的信号量id。

    if (semlock.init(0x5095) == false)  // 初始化信号量。
    {
        printf("创建/获取信号量(%x)失败。\n",0x5095); EXIT(-1);
    }

    semlock.wait();              // 给共享内存加锁。

    // 进程id是循环使用的，如果曾经有一个进程异常退出，没有清理自己的心跳信息，
    // 它的进程信息将残留在共享内存中，不巧的是，如果当前进程重用了它的id，
    // 所以，如果共享内存中已存在当前进程编号，一定是其它进程残留的信息，当前进程应该重用这个位置。
    for (int ii=0;ii<1000;ii++)
    {
        if ( m_shm[ii].pid==procinfo.pid ) 
        { 
            m_pos=ii; 
            printf("找到旧位置ii=%d\n",ii);
            break; 
        }
    }

    if (m_pos==-1)
    {
        // 在共享内存中寻找一个空的位置，把当前进程的结构体保存到共享内存中。
        for (int ii=0;ii<1000;ii++)
            if ( m_shm[ii].pid==0 )  // 如果pid是空的，表示这是一个空位置。
            { 
                m_pos=ii; 
                printf("找到新位置ii=%d\n",ii);
                break; 
            }
    }

    // 如果m_pos==-1，表示没找到空位置，说明共享内存的空间已用完。
    if (m_pos==-1) 
    { 
        semlock.post();    ("共享内存空间已用完。\n");  EXIT(-1); 
    }

    // 把当前进程的结构体保存到共享内存中。
    //memcpy(m_shm+m_pos,&procinfo,sizeof(struct st_procinfo)); 
    memcpy(&m_shm[m_pos],&procinfo,sizeof(struct st_procinfo)); 

    semlock.post();       // 解锁。

    while (1)
    {
        printf("服务程序正在运行中...\n");
        // 更新进程的心跳信息。
        sleep(25);
        m_shm[m_pos].atime=time(0);
        sleep(25);
        m_shm[m_pos].atime=time(0);
    }

    return 0;
}

// 程序退出和信号2、15的处理函数。
void EXIT(int sig)
{
    printf("sig=%d\n",sig);

    // 从共享内存中删除当前进程的心跳信息。
    if (m_pos!=-1) memset(m_shm+m_pos,0,sizeof(struct stprocinfo)); 

    // 把共享内存从当前进程分离。
    if (m_shm!=0) shmdt(m_shm);

    exit(0);
}
