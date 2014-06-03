/*
 * sample.c
 *
 * 2014@Taiwan
 *
 * author: BeJo Li
 * mail: bejo.mob@gmail.com
 */

#include <config.h>
#include <sppCtrl.h>

static void help(void);
static char *help_str[] = {
"Example:\n"
"\t[CMD] sample off\n"
"Command:\n"
};

typedef void (*FUNC)(void);

char *sample_status(void)
{
    static char tmp[] = "spp_sample=on\n";

//    DBGMSG("\nbacktick sample = \n%s\n", backticksh("ifconfig %s", "eth2"));

    return tmp;
}


static void set_off(void)
{
#ifdef X86_TEST
#endif
    SPP_PRINT("Set sample OFF\n");
}

static void set_on(void)
{
#ifdef X86_TEST
#endif
    SPP_PRINT("Set sample ON\n");
}

static void *cmd[CMD_NUM][CMD_LEN] = {
    {"help", "Show this help page", &help},
    {"off", "Turn off sample", &set_off},
    {"on", "Turn on sample", &set_on},
    {NULL, NULL, NULL}
};

static void help(void)
{
    int i = 0;
    SPP_PRINT("%s", help_str[0]);
    for (i = 0; cmd[i][0]; i++) {
        SPP_PRINT("%s,       \t%s\n", (char *)cmd[i][0], (char *)cmd[i][1]);
    }

}

int sample(int argc, char **argv)
{
    int cmdVector = 0;

    if (argc < 3) {
        help();
        return SPP_FAIL;
    }

    cmdVector = sppcmd_check(cmd, argv[2]);
    if (cmdVector == SPP_FAIL) {
        help();
        return SPP_FAIL;
    }

    if (cmd[cmdVector][0] != NULL) {
        ((FUNC)cmd[cmdVector][2])();    
    } else {
        help();
        return SPP_FAIL;
    }
    return SPP_OK;
}

