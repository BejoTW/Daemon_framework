/*
 * config.h
 *
 * author: BeJo Li
 * mail: bejo.mob@gmail.com
 *
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#if 0
#define DBGMSG(fmt, args...) fprintf(stderr, "[%s] %s(%d): " fmt, __FILE__,__FUNCTION__, __LINE__, ##args)
#else
#define DBGMSG(fmt, args...)
#endif

#define SPP_OK  1
#define SPP_FAIL    -1
#define SPP_PRINT(fmt, args...) printf(fmt, ##args)
#define PID_FILE    "/tmp/spp.pid"

#ifdef X86_TEST
#define SPP_EXEC(fmt, args...) ({printf("[JUST PRINT]" fmt"\n", ##args); strdup("Just Print on X86\n");})
#else
//#define SPP_EXEC    backticksh
#define SPP_EXEC(fmt, args...) ( \
{\
    char buf[1024];\
    bzero(buf, sizeof(buf));\
    sprintf(buf, fmt, ##args);\
    system(buf);})
#endif

#define CMD_NUM 256
#define CMD_LEN 3
#define CMD_VER "0.1"

#define STATUS_BUF 256

#endif /* __CONFIG_H__ */
