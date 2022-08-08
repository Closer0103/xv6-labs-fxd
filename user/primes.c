#include "kernel/types.h"
#include "user/user.h"
#include "stddef.h"

void mapping(int n, int tmp[])//对文件描述符进行重新映射
{
  close(n);
  dup(tmp[n]);
  close(tmp[0]);
  close(tmp[1]);
}

void primes()//求素数的函数
{
  int previous, next;
  int fd[2];
  if (read(0, &previous, sizeof(int)))//在管道中读取素数
  {
    printf("prime %d\n", previous);
    pipe(fd);
    if (fork() == 0)//创建一个子进程
    {
      mapping(1, fd);
      while (read(0, &next, sizeof(int)))//循环读取管道中的数据
      {
        if (next % previous != 0)
        {
          write(1, &next, sizeof(int));
        }
      }
    }
    else
    {
    //父进程，等待子进程将数据全部写入管道之中
      wait(NULL);
      mapping(0, fd);
      primes();
    }  
  }  
}

int main(int argc, char *argv[])
{
  int fd[2];
  pipe(fd);//完成管道的创建
  if (fork() == 0)
  {
  //子进程
    mapping(1, fd);
    for (int i = 2; i < 36; i++)
    {
      write(1, &i, sizeof(int));//循环获取2至35的数据，并将它们写入管道当中
    }
  }
  else
  {
    wait(NULL);
    mapping(0, fd);
    primes();//调用函数求取素数
  }
  exit(0);
}
