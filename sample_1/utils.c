/*
 * utils.c
 *
 * 2014@Taiwan
 *
 * author: BeJo Li
 * mail: bejo.mob@gmail.com
 */

#include <config.h>
#include <sppCtrl.h>
#include <utils.h>

char read_proc_by_char(char *path)
{
    char state = '0';
    FILE *fp = NULL;
    
    fp = fopen(path, "r");

    if (fp) {
        state = fgetc(fp);
        fclose(fp);
    } else {
        DBGMSG("File:%s open fail\n", path);
    }
    return state;
}

int sppcmd_check(void *cmd_tables[CMD_NUM][CMD_LEN], char *cmd)
{
    int i = 0;
    int match = 0;
    int cmd_index = -1;
    
    for (i = 0; cmd_tables[i][0]; i++){
        DBGMSG("cmd_tables[i][0] = %s, cmd = %s\n", cmd_tables[i][0], cmd);
        if(!strncmp(cmd_tables[i][0], cmd, strlen(cmd))){
            match++;
            cmd_index = i;
        }
    }
    if (match == 1) {
        return cmd_index;
    } else {
        return SPP_FAIL;
    }
}

char *str_replace(char *orig, char *rep, char *with) 
{
    char *result, *ins, *tmp; 
    int len_rep, len_with, len_front;
    int count;

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}
