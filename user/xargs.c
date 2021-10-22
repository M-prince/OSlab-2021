#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"


int main(int argc, char *argv[]) 
{ 
    char* params[MAXARG];
    char buf[1024];
    if(argc < 2){
        printf("Error: xargs command\n");
        exit(1);
    }
    if(argc + 1 > MAXARG){
        printf("Error: too many args\n");
        exit(1);
    }
    for (int i = 1; i < argc; i++)
    {
        params[i-1] = argv[i];
    }
    params[argc] = 0; 
    while(1)
    {
        int i;    
        for(i = 0; !((read(0, &buf[i], 1) == 0) || (buf[i] == '\n')); i++);  
        if(i == 0) break;   //Ctrl + D
        buf[i] = 0;     //set the end signal
        params[argc -1] = buf;
        if(fork() == 0){
            exec(params[0], params);
            exit(0);
        }
        else wait(0);
    }
    exit(0);
}
