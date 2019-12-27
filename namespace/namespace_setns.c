
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static char child_stack[2048*1024];

int child_func(void *args)
{
	char *hostname = (char*)args;

	sethostname(hostname, strlen(hostname));
	execlp("bash", "bash", (char *) NULL);

	//unreachable
	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int child_pid;

	if (argc < 2) {
		printf("Usage: %s </proc/PID/ns/file>\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		perror("open");
		exit(-1);
	}

	if (setns(fd, 0) < 0) {
		perror("setns");
		exit(-1);
	}

	
	execlp("bash", "bash", (char *) NULL);

	child_pid = clone(child_func, child_stack + sizeof(child_stack),
		CLONE_NEWUTS | CLONE_NEWPID | SIGCHLD, argv[1]);

	if (child_pid < 0) {
		perror("clone");
		exit(-1);
	}

	printf("PID of child created by clone() is %ld\n", (long) child_pid);
	waitpid(child_pid, NULL, 0); //等待子进程结束
	printf("PID of child %ld exited\n", (long) child_pid);
	return 0;
}

