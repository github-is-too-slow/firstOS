#ifndef _ORANGES_STDIO_H_
#define _ORANGES_STDIO_H_
#define O_CREATE 1
#define O_RDWR 2
#define	STR_DEFAULT_LEN	1024      //格式化输出缓冲区大小

/*库函数*/
PUBLIC int open(char *pathname, int flags);
PUBLIC int close(int fd);
PUBLIC int read(int fd, void *buf, int count);
PUBLIC int write(int fd, const void *buf, int count);
PUBLIC int unlink(char * pathname);
PUBLIC int print(const char *fmt, ...);
#endif