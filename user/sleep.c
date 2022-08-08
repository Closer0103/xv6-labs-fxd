#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc != 2)//如果命令行参数不等于2个，打印错误信息
    {
        write(2, "输入参数：睡眠时间\n", strlen("输入参数：睡眠时间\n"));
        exit(1);
    }
    else
    {
    int time = atoi(argv[1]);//字符串转INT型
    sleep(time);//系统调用SLEEP函数
    exit(0);
    }
}
