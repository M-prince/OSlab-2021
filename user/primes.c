#include "kernel/types.h"
#include "user.h"

int fun(int p[]);

int main()
{
    int p[2];
    if(pipe(p) != 0)
    {
        printf("pipe error\n");
        exit(1);
    } 
    //Initialize
    for( int i = 2; i <= 35; i++)
    {
        write(p[1], &i, sizeof(int));
    }
    close(p[1]);
    fun(p);
    exit(0);  
}

int fun(int p[])
{
    int pid;
    int buf, prime;
    int pp[2];
    close(p[1]);
    if(read(p[0], &prime, sizeof(int)) == 0)   //reach the end of pipe
    {
        close(p[0]);
        exit(0);
    }
    else
    {
        printf("prime %d\n", prime);
        if(pipe(pp) != 0)
        {
            printf("pipe error\n");
            exit(1);
        }  
        switch (pid = fork())
        {
        case -1:
            printf("fork error\n");
            exit(1);
            break;
        case 0:
            close(pp[1]);
            fun(pp);
            break;
        default:
            close(p[1]);
            close(pp[0]);
            for(int i = 1; read(p[0], &buf, sizeof(int)) != 0; i++)
            {
                if((buf % prime) != 0)
                write(pp[1], &buf, sizeof(int));
                
            }
            close(pp[1]);
            close(p[0]);
            wait(&pid);
            break;
        }       
    }
    exit(0);
}