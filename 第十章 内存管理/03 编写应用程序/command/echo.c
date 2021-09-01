#include "stdio.h"

int main(int argc, char * argv[])
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	int i;
	for (i = 1; i < argc; i++)
        //省略echo命令名
		print("%s%s", i == 1 ? "" : " ", argv[i]);
	print("\n");
	close(fd_stdin);
	close(fd_stdout);
	return 0;
}