#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>

char output_file[400];
int output_file_flag = 0;
char preload_file[400] = "./logger.so";
char *exe_args[50];
int exe_idx = 0;

int main(int argc, char* argv[]){
    if(argc < 2) {
        printf("no command given.\n");
        return 0;
    }

    char c;
    while((c = getopt(argc, argv, "o:p:")) != -1) {
        switch(c) {
            case 'o':
                strcpy(output_file, optarg);
                output_file_flag = 1;
                break;
            case 'p':
                strcpy(preload_file, optarg);
                break;
            default:
                printf("usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]\n");
                printf("       -p: set the path to logger.so, default = ./logger.so\n");
                printf("       -o: print output to file, print to \"stderr\" if no file specified\n");
                printf("       --: separate the arguments for logger and for the command\n");
                break;
        }
    }

    for(int i = optind; i < argc; i++) {
        exe_args[exe_idx] = argv[i];
        exe_idx++;
    }

    pid_t pid = fork();
   
    if(pid == 0) {
        if(output_file_flag == 1){
            char fd_buffer[10];
            int fd = open(output_file, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
            snprintf(fd_buffer, sizeof(fd_buffer), "%d", fd);
            setenv("output_file_fd", fd_buffer, 1);
        }
        setenv("LD_PRELOAD", preload_file, 1);
        if(execvp(argv[optind], exe_args) == -1){
            exit(0);
        }
    }else if(pid > 0) {
        while(waitpid(pid, NULL, WNOHANG) != -1);
    }

    return 0;
}