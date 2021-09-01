#include "type.h"
#include "stdio.h"

int main(int argc, char * argv[])
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	print("/\n");
	return 0;
}