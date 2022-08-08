#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

void find(char *dir, char *file)
{   
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(dir, 0)) < 0)//打开路径
    {
        fprintf(2, "find: 无法打开文件 %s\n", dir);
        return;
    }

    if (fstat(fd, &st) < 0)//系统调用fstat
    {
        fprintf(2, "find: cannot stat %s\n", dir);
        close(fd);
        return;
    }

    if (st.type != T_DIR)//如果不是目录类型
    {
        fprintf(2, "find: %s 不是目录类型\n", dir);
        close(fd);
        return;
    }

    if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf)//若路径太长，而无法放入缓冲区中
    {
        fprintf(2, "find: 路径过长\n");
        close(fd);
        return;
    }
    strcpy(buf, dir);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de))//读取，如果返回字节数相等则进行循环
    {
        if(de.inum == 0)
            continue;
        if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0)//若出现问题，则直接报错
        {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }
        if (st.type == T_DIR)
        {
            find(buf, file);
        }
        else if (st.type == T_FILE && !strcmp(de.name, file))
        {
            printf("%s\n", buf);
        } 
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)//参数错误报错
    {
        fprintf(2, "usage: find dirName fileName\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
