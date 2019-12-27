
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/utsname.h>

static char child_stack[2048*1024];

int child_func(void *args)
{
	struct utsname uts;
	char *hostname = (char*)args;

	printf("I'm a child process, my pid %d, parent pid: %d\n", getpid(), getppid());
	sethostname(hostname, strlen(hostname));
	uname(&uts);
	if (uname(&uts) == -1) {
		perror("uname");
		exit(-1);
	}
	printf("uts.nodename in child:  %s\n", uts.nodename);
	execlp("bash", "bash", (char *) NULL);

	//unreachable
	return 0;
}

int main(int argc, char **argv)
{
	int child_pid;

	if (argc < 2) {
		printf("Usage: %s <child-hostname>\n", argv[0]);
		return -1;
	}

	child_pid = clone(child_func, child_stack + sizeof(child_stack),
		CLONE_NEWUTS | SIGCHLD, argv[1]);

	if (child_pid < 0) {
		perror("clone");
		exit(-1);
	}

	printf("PID of child created by clone() is %ld\n", (long) child_pid);
	waitpid(child_pid, NULL, 0); //等待子进程结束
	printf("PID of child %ld exited\n", (long) child_pid);
	return 0;
}

