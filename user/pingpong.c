#include "kernel/types.h"
#include "user.h"

int main()
{
    int p1[2];
    int p2[2];
    //the 0 side of a pipe means read, 1 means write
    int pid;
    char s1[] = "ping";
    char s2[] = "pong";
    int s_s1 = sizeof(s1);
    int s_s2 = sizeof(s2);
    if(pipe(p1) != 0 || pipe(p2) != 0)
    {
        printf("pipe error\n");
        exit(1);
    }

    if((pid = fork()) < 0)
    {
        printf("fork error\n");
        exit(1);
    }
    
    else if(pid == 0)    //child
    {
        close(p1[1]);
        if(read(p1[0], s1, s_s1) != s_s1)
        {
            printf("child read error\n");
            exit(1);
        }
        printf("4: received ping\n");   
        close(p1[0]);
        close(p2[0]);
        if (write(p2[1], s2, s_s2) != s_s2)
        {
            printf("child write error\n");
            exit(1);
        }
        close(p2[1]);
        exit(0);
    }

    else    //parent
    {
        close(p2[1]);
        close(p1[0]);
        if (write(p1[1], s1, s_s1) != s_s1)
        {
            printf("child write error\n");
            exit(1);
        }
        close(p1[1]);
        if(read(p2[0], s2, s_s2) != s_s2)
        {
            printf("parent read error\n");
            exit(1);
        }
        printf("3: received pong\n");
        exit(0);   
    }
    exit(0);
}
