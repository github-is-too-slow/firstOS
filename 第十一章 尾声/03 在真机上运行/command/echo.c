#include "stdio.h"

int main(int argc, char **argv)
{
	int i;
	char **tmp_argv = argv;
	for (i = 1; i < argc; i++){
		//省略echo命令名
		print("%s%s", i == 1 ? "" : " ", tmp_argv[i]);
	}
	print("\n");
	return 0;
}