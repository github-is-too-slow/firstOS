#include <stdio.h>

//#的作用是将实参传过来的内容变为字符串常量,并且作为字符串指针出现在相应位置
#define SQ(exp) printf(#exp"hello world")


int main() {
    int k_reenter = 0;
    SQ(0\n);  // ==> printf("0\n""hello world");
    SQ(k_reenter == 0);
    SQ((k_reenter == 0));
}