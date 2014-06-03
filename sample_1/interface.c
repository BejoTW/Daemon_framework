/*
 * interface.c
 *
 * 2014@Taiwan
 *
 * author: BeJo Li
 * mail: bejo.mob@gmail.com
 */

#include <config.h>
#include <sppCtrl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define INT_STR 32
#define MANAGE_INT  "br0"
#define MANAGE_AP   "ra0"
#define MANAGE_DID  "ra1"
#define MANAGE_APCLI "apcli0"

static void help(void);
static char *help_str[] = {
"Example:\n"
"\t[CMD] interface off\n"
"Command:\n"
};

typedef void (*FUNC)(void);

typedef struct {
    char if_name[INT_STR];
    char ip_addr[INT_STR];
    char mask[INT_STR];
    char mac[INT_STR];
} ETH_INT;

int GetNetInfo(ETH_INT *eth_int)
{ 
    int ret = 0; 
 
    struct ifreq req; 
    struct sockaddr_in* host = NULL; 
 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (-1 == sockfd ) { 
        return SPP_FAIL; 
    } 
 
    bzero(&req, sizeof(struct ifreq)); 
    strcpy(req.ifr_name, eth_int->if_name); 
    if (ioctl(sockfd, SIOCGIFADDR, &req) >= 0) { 
        host = (struct sockaddr_in*)&req.ifr_addr; 
        sprintf(eth_int->ip_addr, "%s", inet_ntoa(host->sin_addr));
    } 
    else { 
        ret = SPP_FAIL; 
    } 
 
    bzero(&req, sizeof(struct ifreq)); 
    strcpy(req.ifr_name, eth_int->if_name); 
    if (ioctl(sockfd, SIOCGIFNETMASK, &req) >= 0 ) { 
        host = (struct sockaddr_in*)&req.ifr_addr;
        sprintf(eth_int->mask, "%s", inet_ntoa(host->sin_addr)); 
    } 
    else { 
        ret = SPP_FAIL; 
    } 
 
    bzero(&req, sizeof(struct ifreq)); 
    strcpy(req.ifr_name, eth_int->if_name); 
    if (ioctl(sockfd, SIOCGIFHWADDR, &req) >= 0 ) { 
        sprintf(eth_int->mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
            (unsigned char)req.ifr_hwaddr.sa_data[0], 
            (unsigned char)req.ifr_hwaddr.sa_data[1], 
            (unsigned char)req.ifr_hwaddr.sa_data[2], 
            (unsigned char)req.ifr_hwaddr.sa_data[3], 
            (unsigned char)req.ifr_hwaddr.sa_data[4], 
            (unsigned char)req.ifr_hwaddr.sa_data[5] 
        ); 
    } 
    else { 
        ret = SPP_FAIL; 
    } 
 
    if (sockfd != -1 ) { 
        close(sockfd); 
        sockfd = -1; 
    } 

    return ret; 
} 

char *interface_status(void)
{

    ETH_INT eth_int;
    static char status_buf[STATUS_BUF];
    int len = 0;
    int i = 0;

    char *int_query_tbl[] = {
        MANAGE_INT,
        MANAGE_AP,
        MANAGE_DID,
        MANAGE_APCLI,
        NULL
    };

    bzero(&status_buf, sizeof(status_buf));

    for (i = 0; int_query_tbl[i] != NULL; i++) {
        bzero(&eth_int, sizeof(ETH_INT));
        strcpy(eth_int.if_name, int_query_tbl[i]);
        GetNetInfo(&eth_int);

        len += sprintf(status_buf+len, "spp_%s_ip=%s\n", eth_int.if_name, eth_int.ip_addr);
        len += sprintf(status_buf+len, "spp_%s_mask=%s\n", eth_int.if_name, eth_int.mask);
        len += sprintf(status_buf+len, "spp_%s_mac=%s\n", eth_int.if_name, eth_int.mac);
    }

    return status_buf;
}



static void set_off(void)
{
    SPP_PRINT("Set Interface OFF\n");
}

static void set_on(void)
{
    SPP_PRINT("Set Interface ON\n");
}

static void *cmd[CMD_NUM][CMD_LEN] = {
    {"help", "Show this help page", &help},
    {"off", "Turn off interface", &set_off},
    {"on", "Turn on interface", &set_on},
    {NULL, NULL, NULL}
};

static void help(void)
{
    int i = 0;
    SPP_PRINT("%s", help_str[0]);
    for (i = 0; cmd[i][0]; i++) {
        SPP_PRINT("%s,      \t%s\n", (char *)cmd[i][0], (char *)cmd[i][1]);
    }

}

int interface(int argc, char **argv)
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

