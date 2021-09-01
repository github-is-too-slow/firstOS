#ifndef _ORANGES_STDIO_H_
#define _ORANGES_STDIO_H_
#define O_CREATE 1
#define O_RDWR 2

/*库函数*/
PUBLIC int open(char *pathname, int flags);
PUBLIC int close(int fd);
#endif