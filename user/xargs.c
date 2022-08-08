#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXN 1024

int main(int argc, char *argv[])
{
    if (argc < 2) //参数错误提示
    {
        fprintf(2, "usage: xargs command\n");
        exit(1);
    }
    char * argvs[MAXARG];
    int index = 0;
    for (int i = 1; i < argc; ++i) {
        argvs[index++] = argv[i];
    }
    char buf[MAXN] = {"\0"};
    
    int n;
    while((n = read(0, buf, MAXN)) > 0 ) 
    {
		char temp[MAXN] = {"\0"};
        argvs[index] = temp;
        for(int i = 0; i < strlen(buf); ++i) //内循环获取追加参数并创建子进程执行命令
        {
            if(buf[i] == '\n') 
            {
                if (fork() == 0) 
                {
                    exec(argv[1], argvs);
                }
                wait(0);
            } else 
            {
                temp[i] = buf[i];
            }
        }
    }
    exit(0);
}
