void myprint(char *msg, int len); //默认是extern

int choose(int a, int b){
    if(a >= b){//长度包括终结符\0
        myprint("the first one\n", 15);
    }else {
        myprint("the second one\n", 16);
    }
    return 0;
}