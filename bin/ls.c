#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_LS_DIRENTS 128  ///< 最大目录数

struct ls_opt 
{
	bool ls_l;  ///< 是否-l选项
	bool ls_a;  ///< 是否-a选项
	bool ls_h;  ///< 是否-h选项
	char *dirs[MAX_LS_DIRENTS];  ///< 参数目录
	int  dircnt;                 ///< 目录个数
};

/**
 * 初始化
 */
void ls_opt_ctor(struct ls_opt *opt)
{
	opt->ls_l = false;
	opt->ls_a = false;
	opt->ls_h = false;
	opt->dircnt = 0;
	for(int i = 0; i < MAX_LS_DIRENTS; i++)
	{
		opt->dirs[i] = NULL;
	}
}

int parse_cmd(int argc, char *argv[], struct ls_opt *opt)
{
	for(int i = 1; i < argc; i++)
	{
		char *src = argv[i];
		if(src[0] == '-')
		{
			int len = strlen(src);
			for(int i = 1; i < len; i++)
			{
				switch(src[i])
				{
					case 'a':
						opt->ls_a = true;
						break;
					case 'l':
						opt->ls_l = true;
						break;
					case 'h':
						opt->ls_h = true;
						break;
				}
			}
		}else 
		{
			if(opt->dircnt >= MAX_LS_DIRENTS){

				printf("too many dirs\n");
				return -1;
			}
			opt->dirs[opt->dircnt++] = src;
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	printf("in ls\n");
	printf("argc %d\n", argc);
	printf("argv[0] %s\n", argv[0]);
	printf("argv[1] %s\n", argv[1]);
	struct ls_opt opt;
	ls_opt_ctor(&opt);
	if(parse_cmd(argc, argv, &opt) == -1)
	{
		printf("parse cmd error\n");
		return -1;
	}
	DIR *dir = NULL;
	struct dirent *ent;
	for(int i = 0; i < opt.dircnt; i++)
	{
		if((dir = opendir(opt.dirs[i])) == NULL)
		{
			printf("opendir %s failed\n", opt.dirs[i]);
			continue;
		}
		printf("%s:", opt.dirs[i]);
		for(int j = 0; ((ent = readdir(dir)) != NULL);) 
		{
			if(ent->d_name[0] == '.' && !opt.ls_a)
				continue;

			if(j++ % 5 == 0)
				printf("\n");

			printf("%-10s", ent->d_name);
		}
	}
	return 0;
}	

