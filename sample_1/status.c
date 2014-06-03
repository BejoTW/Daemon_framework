/*
 * status.c
 *
 * 2014@Taiwan
 *
 * author: BeJo Li
 * mail: bejo.mob@gmail.com
 */

#include <config.h>
#include <sppCtrl.h>

#include <feature_set.h>

#define STATUS_FILE_PATH    "/tmp/spp_status"
#define STATUS_FILE_PATH_PRE    "/tmp/spp_status_"
#define STATUS_FILE_NAME_LEN    64

static int help(int, char **);
static char *help_str[] = {
"Example:\n"
"\t[CMD] status update\n"
"Command:\n"
};

typedef int (*FUNC)(int, char **);
typedef char *(*FUNC_STATUS)(void);

static char *list_status(void);

static void *status_tables[][2] = {
    {"interface", &interface_status},
    {"sample", &sample_status},
    {"help", &list_status},
    {NULL, NULL}
};

static char *list_status(void)
{
    int i = 0;
    SPP_PRINT("\nUpdate feature support:\n");
    for (i = 0; status_tables[i][0]&&strcmp("help", status_tables[i][0]); i++) {
        SPP_PRINT("\t%s \n", (char *)status_tables[i][0]);
    }
}

static int update(int argc, char **argv)
{
    DBGMSG("update status\n");
    FILE    *fp = NULL;
    int i = 0;
    char path[STATUS_FILE_NAME_LEN] = STATUS_FILE_PATH_PRE;

    switch(argc) {
        case 5:
            fp = fopen(strcat(path, argv[4]), "w+");
            if (fp == NULL) {
                SPP_PRINT("Status file %s open fail\n", argv[4]);
                return SPP_FAIL;
            }
        case 4:
            if (fp == NULL) {
                fp = fopen(STATUS_FILE_PATH, "w+");
                if (fp == NULL) {
                    SPP_PRINT("Status file %s open fail\n", STATUS_FILE_PATH);
                    return SPP_FAIL;
                }
            }
            for (i = 0; status_tables[i][0]; i++) {
                if (!strcmp(argv[3], status_tables[i][0])) {
                    break;
                }
            }
            if (status_tables[i][0] != NULL) {
                fprintf(fp, "%s",((FUNC_STATUS)status_tables[i][1])());
            } else {
                help(argc, argv);
            }
            break;
        case 3:
            fp = fopen(STATUS_FILE_PATH, "w+");
            if (fp == NULL) {
                SPP_PRINT("Status file %s open fail\n", STATUS_FILE_PATH);
                return SPP_FAIL;
            }
            for (i = 0; status_tables[i][0]; i++) {
                if (status_tables[i][0] != NULL&&strcmp("help", status_tables[i][0])) {
                fprintf(fp, "%s",((FUNC_STATUS)status_tables[i][1])());
                }
            }
            break;
        default:
            list_status();
    }
    if (fp != NULL) {
        fclose(fp);
    }
    return SPP_OK;
}

static void *cmd[CMD_NUM][CMD_LEN] = {
    {"help", "Show this help page", &help},
    {"update", "update status ex: update [Feature] or update [Feature] \
<"STATUS_FILE_PATH_PRE"YOUR_FILE_NAME>", &update},
    {NULL, NULL, NULL}
};

static int help(int argc, char **argv)
{
    int i = 0;
    SPP_PRINT("%s", help_str[0]);
    for (i = 0; cmd[i][0]; i++) {
        SPP_PRINT("%s,       \t%s\n", (char *)cmd[i][0], (char *)cmd[i][1]);
    }

}

int status(int argc, char **argv)
{
    int cmdVector = 0;

    if (argc < 3) {
        help(argc, argv);
        return SPP_FAIL;
    }

    cmdVector = sppcmd_check(cmd, argv[2]);
    if (cmdVector == SPP_FAIL) {
        help(argc, argv);
        return SPP_FAIL;
    }

    if (cmd[cmdVector][0] != NULL) {
        ((FUNC)cmd[cmdVector][2])(argc, argv);
    } else {
        help(argc, argv);
        return SPP_FAIL;
    }
    return SPP_OK;
}

