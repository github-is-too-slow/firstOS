#include <stdio.h>
#define __DEBUG__
#ifdef __DEBUG__
#define DEBUG(format,...) printf("File: "__FILE__", Line: %05d, "format"\n", __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format,...)
#endif

#line 200

int main() {
    char str[]="Hello World";
    DEBUG("A ha, check me: %s",str);
    printf(__BASE_FILE__"\n");
    printf("%s\n",__DATE__);
	printf("%s\n",__TIME__);
    printf("%s\n",__FUNCTION__);
    printf("%s\n",__func__);
    printf("hello ""world\n");
    //非法：printf("hello "+"world\n");
    //cause: invalid operands to binary + (have 'char *' and 'char *')
    return 0;
}