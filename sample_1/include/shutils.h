/*
 * Shell-like utility functions
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#ifndef _shutils_h_
#define _shutils_h_

#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#ifdef REDIRECT_NULL_DEVICE
#define NULL_DEVICE ">/dev/null"
#else
#define NULL_DEVICE NULL
#endif /* REDIRECT_NULL_DEVICE */

#ifndef SAFE_FREE
#define SAFE_FREE(p) {\
	if( p != NULL )\
	{\
		free(p);\
		p = NULL;\
	}\
}
#endif

#ifndef ROUNDUP
#define  ROUNDUP(x, y)       ((((x)+((y)-1))/(y))*(y))
#endif

#ifndef MIN
#define  MIN(a, b)               (((a)<(b))?(a):(b))
#endif
	
/*
 * Reads file and returns contents
 * @param	fd	file descriptor
 * @return	contents of file or NULL if an error occurred
 */
extern char * fd2str(int fd);

/*
 * Reads file and returns contents
 * @param	path	path to file
 * @return	contents of file or NULL if an error occurred
 */
extern char * file2str(const char *path);

/* 
 * Waits for a file descriptor to become available for reading or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */
extern int waitfor(int fd, int timeout);

/* 
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */
extern int _eval(char *const argv[], char *path, int timeout, pid_t *ppid);

/* 
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @return	stdout of executed command or NULL if an error occurred
 */
extern char * _backtick(char *const argv[]);

/* 
 * Signal process whose PID is stored in plaintext in pidfile
 * @param	pidfile	PID file
 * @param	Signal number
 * @return	0 on success and errno on failure
 */
extern int signal_pidfile(char *pidfile, int signo);

/* 
 * Kills process whose PID is stored in plaintext in pidfile
 * @param	pidfile	PID file
 * @return	0 on success and errno on failure
 */
extern int kill_pidfile(char *pidfile);

/*
 * fread() with automatic retry on syscall interrupt
 * @param	ptr	location to store to
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully read
 */
extern int safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
extern int safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/*
 * Convert Ethernet address string representation to binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @param	e	binary data
 * @return	TRUE if conversion was successful and FALSE otherwise
 */
extern int ether_atoe(const char *a, unsigned char *e);

/*
 * Convert Ethernet address binary data to string representation
 * @param	e	binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @return	a
 */
extern char * ether_etoa(const unsigned char *e, char *a);

/*
 * Concatenate two strings together into a caller supplied buffer
 * @param	s1	first string
 * @param	s2	second string
 * @param	buf	buffer large enough to hold both strings
 * @return	buf
 */
static inline char * strcat_r(const char *s1, const char *s2, char *buf)
{
	strcpy(buf, s1);
	strcat(buf, s2);
	return buf;
}

/* 
 * shell execution with _eval
 * @param	fmt	argument string
 * @return	return value of executed command or errno
 */
extern int evalsh(const char *fmt,...);

extern int evalsh_nowait(const char *fmt,...);

/* 
 * shell execution with _backtick
 * @param	fmt	argument string
 * @return	return character buffer string of executed command or NULL. Should free() return pointer.
 */
extern char *backticksh(const char *fmt,...);



/* Check for a blank character; that is, a space or a tab */
#define isblank(c) ((c) == ' ' || (c) == '\t')

/* Strip trailing CR/NL from string <s> */
#define chomp(s) ({ \
	char *c = (s) + strlen((s)) - 1; \
	while ((c > (s)) && (*c == '\n' || *c == '\r')) \
		*c-- = '\0'; \
	s; \
})

/* Simple version of _backtick() */
#define backtick(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_backtick(argv); \
})

/* Simple version of _eval() (no timeout and wait for child termination) */
#define eval(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_eval(argv, NULL_DEVICE, 0, NULL); \
})

#define eval_wait(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	pid_t pid;\
	int ret = 0;\
	if(!(ret = _eval(argv, NULL_DEVICE, 0, &pid)))\
		waitpid(pid, &ret, 0);\
})



#define eval_background(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	pid_t pid;\
	_eval(argv, NULL_DEVICE, 0, &pid);\
})

#define _evalsh(cmd, args...)({ \
	char *argv[] = { "sh", "-c", cmd, ## args, NULL }; \
	_eval(argv, NULL_DEVICE, 0, NULL); \
})

#define _evalsh_nowait(cmd, args...)({ \
	char *argv[] = { "sh", "-c", cmd, ## args, NULL }; \
	_eval_nowait(argv, NULL_DEVICE, 0, NULL); \
})

#define _evalsh2(cmd, args...)({ \
	char *argv[] = { "sh", "-c", cmd, ## args, NULL }; \
	_eval2(argv, NULL_DEVICE, 0, NULL); \
})

#define _evalsh_nowait2(cmd, args...)({ \
	char *argv[] = { "sh", "-c", cmd, ## args, NULL }; \
	_eval_nowait2(argv, NULL_DEVICE, 0, NULL); \
})

#define _backticksh(cmd, args...)({ \
	char *argv[] = { "sh", "-c", cmd, ## args, NULL }; \
	_backtick(argv); \
})

/* Copy each token in wordlist delimited by space into word */
#define foreach(word, wordlist, next) \
	for (next = &wordlist[strspn(wordlist, " ")], \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '); \
	     strlen(word); \
	     next = next ? &next[strspn(next, " ")] : "", \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '))

/* Copy each token in wordlist delimited by delim into word */
#define foreachby(word, wordlist, delim, next) \
	for (next = &wordlist[strspn(wordlist, delim)], \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, delim)] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strstr(next, delim); \
	     strlen(word); \
	     next = next ? &next[strspn(next, delim)] : "", \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, delim)] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strstr(next, delim))

/* Return NUL instead of NULL if undefined */
#define safe_getenv(s) (getenv(s) ? : "")

/* 
 * get content of index by delim from src string
 * @param	src	argument string
 * @return	content	content of index
 * @param	delmin	delmin token
 * @param  index get index value
 * @return	return 0 on success and -1 on failure 
 */
extern int getContentOfIndexByDelim(char *src, char *content, char *delim, int index);

#ifdef linux
/* Print directly to the console */

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
		fprintf(fp, fmt, ## args); \
		fclose(fp); \
	} \
} while (0)
#endif

/* Debug print */
#ifdef DEBUG
#define dprintf(fmt, args...) cprintf("%s: " fmt, __FUNCTION__, ## args)
#else
#define dprintf(fmt, args...)
#endif /*DEBUG*/


#endif /* _shutils_h_ */

