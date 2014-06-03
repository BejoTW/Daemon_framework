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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <shutils.h>

/* Linux specific headers */
#ifdef linux
#include <error.h>
#include <termios.h>
#include <sys/time.h>
#include <net/ethernet.h>
#endif /* linux */
#include <time.h>

#ifdef linux
/*
 * Reads file and returns contents
 * @param	fd	file descriptor
 * @return	contents of file or NULL if an error occurred
 */
char *
fd2str(int fd)
{
	char *buf = NULL;
	size_t count = 0, n;

	do {
		buf = realloc(buf, count + 512);
		n = read(fd, buf + count, 512);
		if (n < 0) {
			free(buf);
			buf = NULL;
		}
		count += n;
	} while (n == 512);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*
 * Reads file and returns contents
 * @param	path	path to file
 * @return	contents of file or NULL if an error occurred
 */
char *
file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		perror(path);
		return NULL;
	}

	return fd2str(fd);
}

/* 
 * Waits for a file descriptor to change status or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */
int
waitfor(int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}

/* 
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */
int
_eval(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;

	switch (pid = fork()) {
	case -1:	/* error */
		perror("fork");
		return errno;
	case 0:		/* child */
		/* Reset signal handlers set for parent process */
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		/* Clean up */
		ioctl(0, TIOCNOTTY, 0);
		close(STDIN_FILENO);
		setsid();

		/* Redirect stdout/stderr to <path> */
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				/* append to <path> */
				flags |= O_APPEND;
				path += 2;
			} else if (!strncmp(path, ">", 1)) {
				/* overwrite <path> */
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDERR_FILENO);				
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}
		}

		/* execute command */
		dprintf("%s\n", argv[0]);
		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		alarm(timeout);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:	/* parent */
		if (ppid) {
			*ppid = pid;
			return 0;
		} else {
			if (waitpid(pid, &status, 0) == -1) {
				/*perror("waitpid");*/
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */
int _eval_nowait(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;
	int i;

	/*
	 *	Perry:
	 *		We need to execute a background process and don't want it to become zombie.
	 *		That's why I create a _eval_nowait to fork twice.
	 */
	switch(pid = fork()) {
		case -1:	/* error */
			perror("fork");
			return errno;
		case 0:		/* child */

			/*
			 *	Fork again and let the parent of this fork exit immediately.
			 */
			switch(pid = fork()) {
				case -1:
					perror("fork");
					return errno;
				case 0:
					/* grandchild */

					/* Reset signal handlers set for parent process */
					for (sig = 0; sig < (_NSIG-1); sig++)
						signal(sig, SIG_DFL);

					/* Clean up */
					ioctl(0, TIOCNOTTY, 0);
					close(STDIN_FILENO);
					for(i=getdtablesize();i>=3;--i) close(i);

					setsid();

					/* Redirect stdout/stderr to <path> */
					if (path) {
						flags = O_WRONLY | O_CREAT;
						if (!strncmp(path, ">>", 2)) {
							/* append to <path> */
							flags |= O_APPEND;
							path += 2;
						} else if (!strncmp(path, ">", 1)) {
							/* overwrite <path> */
							flags |= O_TRUNC;
							path += 1;
						}
						if ((fd = open(path, flags, 0644)) < 0)
							perror(path);
						else {
							dup2(fd, STDERR_FILENO);				
							dup2(fd, STDOUT_FILENO);
							close(fd);
						}
					}

					/* execute command */
					dprintf("### %s\n", argv[0]);
					setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
					alarm(timeout);
					execvp(argv[0], argv);
					perror(argv[0]);
					exit(errno);

				default:
					/* Child leaves */
					exit(0);
			}
		default:	/* parent */
			/* Wait child ... */
			if (waitpid(pid, &status, 0) == -1) {
				//perror("waitpid");
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
	}
	return 0;
}

/*
 * Perry: Some process need stand in as input, so have _eval2 and remove close function
 *
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */
int
_eval2(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;

	switch (pid = fork()) {
	case -1:	/* error */
		perror("fork");
		return errno;
	case 0:		/* child */
		/* Reset signal handlers set for parent process */
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		/* Clean up */
		ioctl(0, TIOCNOTTY, 0);
		//close(STDIN_FILENO); // Perry: Remove this
		setsid();

		/* Redirect stdout/stderr to <path> */
		if (path) {
			flags = O_WRONLY | O_CREAT;
			if (!strncmp(path, ">>", 2)) {
				/* append to <path> */
				flags |= O_APPEND;
				path += 2;
			} else if (!strncmp(path, ">", 1)) {
				/* overwrite <path> */
				flags |= O_TRUNC;
				path += 1;
			}
			if ((fd = open(path, flags, 0644)) < 0)
				perror(path);
			else {
				dup2(fd, STDERR_FILENO);				
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}
		}

		/* execute command */
		dprintf("%s\n", argv[0]);
		setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
		alarm(timeout);
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(errno);
	default:	/* parent */
		if (ppid) {
			*ppid = pid;
			return 0;
		} else {
			if (waitpid(pid, &status, 0) == -1) {
				/*perror("waitpid");*/
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
		}
	}
}

/*
 * Perry: Some process need stand in as input, so have _eval_nowait2 and remove close function
 *
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */
int _eval_nowait2(char *const argv[], char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status;
	int fd;
	int flags;
	int sig;
	int i;

	/*
	 *	Perry:
	 *		We need to execute a background process and don't want it to become zombie.
	 *		That's why I create a _eval_nowait to fork twice.
	 */
	switch(pid = fork()) {
		case -1:	/* error */
			perror("fork");
			return errno;
		case 0:		/* child */

			/*
			 *	Fork again and let the parent of this fork exit immediately.
			 */
			switch(pid = fork()) {
				case -1:
					perror("fork");
					return errno;
				case 0:
					/* grandchild */

					/* Reset signal handlers set for parent process */
					for (sig = 0; sig < (_NSIG-1); sig++)
						signal(sig, SIG_DFL);

					/* Clean up */
					ioctl(0, TIOCNOTTY, 0);
					//close(STDIN_FILENO); //Perry: Close this one
					for(i=getdtablesize();i>=3;--i) close(i); // Perry: we should still close non-standard fd.

					setsid();

					/* Redirect stdout/stderr to <path> */
					if (path) {
						flags = O_WRONLY | O_CREAT;
						if (!strncmp(path, ">>", 2)) {
							/* append to <path> */
							flags |= O_APPEND;
							path += 2;
						} else if (!strncmp(path, ">", 1)) {
							/* overwrite <path> */
							flags |= O_TRUNC;
							path += 1;
						}
						if ((fd = open(path, flags, 0644)) < 0)
							perror(path);
						else {
							dup2(fd, STDERR_FILENO);				
							dup2(fd, STDOUT_FILENO);
							close(fd);
						}
					}

					/* execute command */
					dprintf("### %s\n", argv[0]);
					setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
					alarm(timeout);
					execvp(argv[0], argv);
					perror(argv[0]);
					exit(errno);

				default:
					/* Child leaves */
					exit(0);
			}
		default:	/* parent */
			/* Wait child ... */
			if (waitpid(pid, &status, 0) == -1) {
				//perror("waitpid");
				return errno;
			}
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
			else
				return status;
	}
	return 0;
}

/* 
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @return	stdout of executed command or NULL if an error occurred
 */
char *
_backtick(char *const argv[])
{
	int filedes[2];
	pid_t pid;
	int status;
	char *buf = NULL;

	/* create pipe */
	if (pipe(filedes) == -1) {
		perror(argv[0]);
		return NULL;
	}

	switch (pid = fork()) {
	case -1:	/* error */
		return NULL;
	case 0:		/* child */
		close(filedes[0]);	/* close read end of pipe */
		dup2(filedes[1], 1);	/* redirect stdout to write end of pipe */
		close(filedes[1]);	/* close write end of pipe */
		execvp(argv[0], argv);
		exit(errno);
		break;
	default:	/* parent */
		close(filedes[1]);	/* close write end of pipe */
		buf = fd2str(filedes[0]);
		waitpid(pid, &status, 0);
		break;
	}
	
	return buf;
}

/* 
 * Signal process whose PID is stored in plaintext in pidfile
 * @param	pidfile	PID file
 * @param	Signal number
 * @return	0 on success and errno on failure
 */
int
signal_pidfile(char *pidfile, int signo)
{
	FILE *fp = fopen(pidfile, "r");
	char buf[256];

	if (fp && fgets(buf, sizeof(buf), fp)) {
		pid_t pid = strtoul(buf, NULL, 0);
		fclose(fp);
		return kill(pid, signo);
  	} else {
		return errno;
	}	
}

/* 
 * Kills process whose PID is stored in plaintext in pidfile
 * @param	pidfile	PID file
 * @return	0 on success and errno on failure
 */
int
kill_pidfile(char *pidfile)
{
	return signal_pidfile(pidfile, SIGTERM);
}

/*
 * fread() with automatic retry on syscall interrupt
 * @param	ptr	location to store to
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully read
 */
int
safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
int
safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

#endif /* linux */

/*
 * Convert Ethernet address string representation to binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @param	e	binary data
 * @return	TRUE if conversion was successful and FALSE otherwise
 */
int
ether_atoe(const char *a, unsigned char *e)
{
	char *c = (char *) a;
	int i = 0;

	memset(e, 0, ETHER_ADDR_LEN);
	for (;;) {
		e[i++] = (unsigned char) strtoul(c, &c, 16);
		if (!*c++ || i == ETHER_ADDR_LEN)
			break;
	}
	return (i == ETHER_ADDR_LEN);
}

/*
 * Convert Ethernet address binary data to string representation
 * @param	e	binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @return	a
 */
char *
ether_etoa(const unsigned char *e, char *a)
{
	char *c = a;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (i)
			*c++ = ':';
		c += sprintf(c, "%02X", e[i] & 0xff);
	}
	return a;
}

/* 
 * shell execution with _eval
 * @param	fmt	argument string
 * @return	return value of executed command or errno
 */
int evalsh(const char *fmt,...)
{
    char buf[4096];
    va_list args;
    int ret = 0;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    va_start(args, fmt);
    ret = _evalsh(buf);
    va_end(args);
    
    return ret;
}

/* 
 * shell execution with _eval
 * @param	fmt	argument string
 * @return	return value of executed command or errno
 */
int evalsh_nowait(const char *fmt,...)
{
    char buf[4096];
    va_list args;
    int ret = 0;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    va_start(args, fmt);
    ret = _evalsh_nowait(buf);
    va_end(args);
    
    return ret;
}

/* 
 * shell execution with _eval
 * @param	fmt	argument string
 * @return	return value of executed command or errno
 */
int evalsh2(const char *fmt,...)
{
    char buf[4096];
    va_list args;
    int ret = 0;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    va_start(args, fmt);
    ret = _evalsh2(buf);
    va_end(args);
    
    return ret;
}

/* 
 * shell execution with _eval
 * @param	fmt	argument string
 * @return	return value of executed command or errno
 */
int evalsh_nowait2(const char *fmt,...)
{
    char buf[4096];
    va_list args;
    int ret = 0;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    va_start(args, fmt);
    ret = _evalsh_nowait2(buf);
    va_end(args);
    
    return ret;
}

/* 
 * shell execution with _backtick
 * @param	fmt	argument string
 * @return	return character buffer string of executed command or NULL. Should free() return pointer.
 */
char *backticksh(const char *fmt,...)
{
    char buf[4096];
    va_list args;
    char *ret;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    va_start(args, fmt);
    ret = _backticksh(buf);
    va_end(args);
    
    return ret;
}

/* 
 * get content of index by delim from src string
 * @param	src	argument string
 * @return	content	content of index
 * @param	delmin	delmin token
 * @param  index get index value
 * @return	return 0 on success and -1 on failure 
 */
int getContentOfIndexByDelim(char *src, char *content, char *delim, int index)
{
	char *next;
	int ret = -1;
	int i = 0;
	
	if (!delim) return -1;
		
	foreachby(content, src, delim, next) {
		if (i == index) { 
			return 0;
		}
		i++;
	}
	
	*content = 0;
		
	return ret;
}

/*
 *  get current time string YYYY-MM-DDThh:mm:ss
 *  @param buf store time string
 *  @param size indicate buf size
 */
void get_current_time(char *buf, int size)
{
	time_t clock;
//	struct tm *tm;
	char tmp[16];
	struct tm tm_time;

	time(&clock);
	memset(tmp, 0, sizeof(tmp));
	//tm = gmtime(&clock);
	memcpy(&tm_time, localtime(&clock), sizeof(tm_time));
	strftime(tmp, sizeof(tmp), "%z", &tm_time);
	if (strcmp(&tmp[1], "0000" )== 0){ 
		strcpy(tmp, "Z");
	}
	tmp[5] = tmp[4];
	tmp[4] = tmp[3];
	tmp[3] = ':';
	strftime(buf, size, "%Y-%m-%dT%H:%M:%S", &tm_time);
	strcat(buf, tmp);
}

/*
 *  get current time string YYYY-MM-DDThh:mm:ss
 *  @return string pointer
 */
char *current_time()
{
	time_t clock;
//	struct tm *tm;
	static char buf[32];
	char tmp[16];
	struct tm tm_time;

	time(&clock);
	memset(tmp, 0, sizeof(tmp));
//	tm = gmtime(&clock);
	memcpy(&tm_time, localtime(&clock), sizeof(tm_time));
	strftime(tmp, sizeof(tmp), "%z", &tm_time);
        if (strcmp(&tmp[1], "0000" )== 0){
                strcpy(tmp, "Z");
        }   
	tmp[5] = tmp[4];
        tmp[4] = tmp[3];
        tmp[3] = ':';
	strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_time);
	strcat(buf, tmp);
	return buf;
}

