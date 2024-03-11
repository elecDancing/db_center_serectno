/*
 *  程序名：demo30.cpp，此程序演示开发框架中采用newdir函数根据绝对路径的文件名或目录名逐级的创建目录。
 *  作者：Prtrick
*/
#include "../_public.h"
using namespace std;
using namespace idc;

int main()
{
    // /tmp/aaa/bbb/ccc/ddd    /tmp    /tmp/aaa    /tmp/aaa/bbb    /tmp/aaa/bbb/ccc   /tmp/aaa/bbb/ccc/ddd
    newdir("/tmp/aaa/bbb/ccc/ddd",false);   // 创建"/tmp/aaa/bbb/ccc/ddd"目录。

    newdir("/tmp/111/222/333/444/data.xml",true);   // 创建"/tmp/111/222/333/444"目录。
}

