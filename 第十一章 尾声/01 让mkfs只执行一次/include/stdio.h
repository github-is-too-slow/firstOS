#ifndef _ORANGES_STDIO_H_
#define _ORANGES_STDIO_H_
#define	O_CREATE		1
#define	O_RDWR		2
#define	O_TRUNC		4

#define SEEK_SET	1
#define SEEK_CUR	2
#define SEEK_END	3
#define	STR_DEFAULT_LEN	1024      //格式化输出缓冲区大小

/*库函数*/
int open(char *pathname, int flags);
int close(int fd);
int read(int fd, void *buf, int count);
int write(int fd, const void *buf, int count);
int unlink(char * pathname);
int print(const char *fmt, ...);
#endif