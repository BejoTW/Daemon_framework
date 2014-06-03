/*
 * sppCtrl.c
 *
 * 2014@Taiwan
 *
 * author: BeJo Li
 * mail: bejo.mob@gmail.com
 */

#include <config.h>
#include <sppCtrl.h>

#include <feature_set.h>

int spp_usage(int, char **);
int version(int, char **);
typedef int (*FUNC)(int, char **);

#define PRE_STR "Version:%s\n"\
"Usage: sppCtrl [OPTION...]\n"\
"Examples:\n"\
"\tsppCtrl help\t#Show help page.\n\n"\
"Command:\n"


void *cmd_tables[CMD_NUM][CMD_LEN] = {
    {"help", "To show this help", &spp_usage},
    {"status", "update system status", &status},
    {"interface", "interface OP", &interface},
    {"version", "show Version", &version},
    {"sample", "I am sample", &sample},
    {NULL, NULL, NULL}
};

int version(int argc, char **argv)
{
    SPP_PRINT("Version: %s\n", CMD_VER);
    return SPP_OK;
}


int spp_usage(int argc, char **argv)
{
    int i = 0;
    
    SPP_PRINT(PRE_STR, CMD_VER);
    for (i = 0; cmd_tables[i][0]; i++) {
        SPP_PRINT("  %s, \t\t%-32s\n", (char *)cmd_tables[i][0], (char *)cmd_tables[i][1]);
    }
    return SPP_OK;
}

void spp_lock(void)
{
    pid_t pid;
    FILE *fp = NULL;
    int timeout = 10;

    // Checking pid for 10s timeout

    while (timeout) {
        if(0 == access(PID_FILE, R_OK)) {
            fp = fopen(PID_FILE, "r");
            if (fp == NULL) {
                SPP_PRINT("PID File %s created fail\n\n", PID_FILE);
                exit(SPP_FAIL);
            }
            if (timeout == 1 ) {
                fscanf(fp, "%d", &pid);
                kill(pid, SIGKILL);
                fclose(fp);
                SPP_PRINT("\nsppCtrl is busy and timeout happend!!!\nKill and do new request\n");
                unlink(PID_FILE);
            }
            timeout--;
            sleep(1);
            continue;
        }
        break;
    }
    fp = fopen(PID_FILE, "w+");
    if (fp == NULL) {
        SPP_PRINT("PID File %s created fail\n\n", PID_FILE);
        exit(SPP_FAIL);
    }

    fprintf(fp, "%d", getpid());
    fclose(fp);
}

int main(int argc, char **argv)
{
    int cmdVector = 0;    

    if (argc <= 1) {
        spp_usage(argc, argv);
        return SPP_FAIL;
    }

    // only allow one request for SPP CTRL
    spp_lock();

    cmdVector = sppcmd_check(cmd_tables, argv[1]);
    if (cmdVector == SPP_FAIL) {
        SPP_PRINT("%s: unrecognized option '%s'\n", argv[0], argv[1]);
        spp_usage(argc, argv);
        SPP_PRINT("\nTry '%s help' for more information.\n", argv[0]);
        unlink(PID_FILE);
        return SPP_FAIL;
    }
    
    if (cmd_tables[cmdVector][2]) {
        ((FUNC)cmd_tables[cmdVector][2])(argc, argv);
    } else {    
        SPP_PRINT("\n%s: Command is not support -- %s\n", argv[0], argv[1]);
        SPP_PRINT("\nTry '%s help' for more information.\n", argv[0]);
    }

    unlink(PID_FILE);
    return SPP_OK;
 
}

