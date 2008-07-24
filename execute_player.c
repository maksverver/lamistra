#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/jail.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static char players_dir[] = "/usr/home/maks/lamistra/players";
static char executable_path[] = "/run";
static const char *app_name;

void exec_jailed(char *dir, int args_size, char *args[])
{
    struct jail jail_descr;
    int jail_id, n;
    char **child_args;
 
    /* Go to jail */   
    jail_descr.version = 0;
    jail_descr.path = dir;
    jail_descr.hostname = "localhost";
    jail_descr.ip_number = inet_addr("127.0.0.1");    
    if(jail(&jail_descr) == -1)
    {
        perror(app_name);
        exit(1);
    }
    
    /* Execute application*/   
    child_args = (char**)malloc(sizeof(char*)*(args_size + 2));
    child_args[0] = executable_path;
    for(n = 0; n < args_size; ++n)
        child_args[n + 1] = args[n];
    execv(executable_path, child_args);
    
    perror(app_name);
    exit(1);
}
    

int main(int argc, char *argv[])
{
    app_name = basename(argv[0]);

    if(argc != 2)
    {
        printf("Usage: %s <player> [argument ...]\n", basename(argv[0]));
        exit(0);
    }
    
    if(chdir(players_dir) != 0)
    {
        perror(app_name);
        exit(1);
    }
    
    /* Abort after 5 minutes. */
    alarm(5*60);

    exec_jailed(argv[1], argc - 2, argv + 2);
    
    return 0;
}
