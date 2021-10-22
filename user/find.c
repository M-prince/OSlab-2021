#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path)
{
    char *p;
    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    return p;
    //There is no need to add the blank
}

void find(char *path, char *file_name)  //Reference to ls.c
{
    char buf[512];
    char *p;
    int fd;
    struct dirent de;
    struct stat st;
    if((fd = open(path ,0)) < 0)   
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0)  
    {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch(st.type){
    case T_FILE:
        if (strcmp(fmtname(path),file_name) == 0)
            printf("%s\n",path);
        break;

    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0 || strcmp(de.name,"..") == 0 || strcmp(de.name,".") == 0)   //ignore . and ..
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            find(buf,file_name);
        }
        break;
    }
    close(fd);
}
int main (int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("Please input 2 arguments\n");
        exit(0);
    }
    else
    {
        find(argv[1], argv[2]);
        exit(0);
    }
}