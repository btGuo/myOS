#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define STDIN_REDIRECT      1  ///< '<'
#define STDIN_REDIRECT_EOF  2  ///< '<<'
#define STDOUT_REDIRECT     4  ///< '>'
#define STDOUT_REDIRECT_APP 8  ///< '>>'

#define STDIN_MASK  ~3
#define STDOUT_MASK ~12

#define MAX_ARGS 64
#define MAX_CMD  16

#define PROMPT_BUFSZ 512
#define CMDLINE_SZ 512

static const char *progs[] = 
{
	"ls", "cd", "exit", "clear", NULL,
};

enum {
	ERROR_FORK,
	ERROR_CD,
	ERROR_PIPE,
	ERROR_FD,
};


struct cmd_unit
{
	int   cu_type;
	char *cu_argv[64];
	char *cu_in_file;
	char *cu_out_file;
	int   cu_argc;
};            

void cu_ctor(struct cmd_unit *cu)
{
	memset(cu, 0, sizeof(struct cmd_unit));
}


void cu_print(struct cmd_unit *cu)
{
	printf("total args: %d\n", cu->cu_argc);
	for(int i = 0; i < cu->cu_argc; i++)
	{
		printf("\t%s\n", cu->cu_argv[i]);
	}
	if(cu->cu_type & STDIN_REDIRECT)
	{
		printf("type %s\n", "STDIN_REDIRECT");

	}
	if(cu->cu_type & STDIN_REDIRECT_EOF)
	{
		printf("type %s\n", "STDIN_REDIRECT_EOF");

	}
	if(cu->cu_type & STDOUT_REDIRECT)
	{
		printf("type %s\n", "STDOUT_REDIRECT");

	}
	if(cu->cu_type & STDOUT_REDIRECT_APP)
	{
		printf("type %s\n", "STDOUT_REDIRECT_APP");
	} 
	
	if(cu->cu_in_file)
	{
		printf("input file %s\n", cu->cu_in_file);
	}
	if(cu->cu_out_file)
	{
		printf("output file %s\n", cu->cu_out_file);
	}
	printf("done\n");
}

int parse_line(char *line, struct cmd_unit *cu_ptr, int cu_size)
{
	char *savep = NULL;
	char *delim = " ";
	char *token = strtok_r(line, delim, &savep);

	int cu_len = 0;
	struct cmd_unit *cu = &cu_ptr[cu_len++];
	cu_ctor(cu);

	do{
		int len = strlen(token);
		if(token[0] == '<' && len == 1)
		{
			if(cu->cu_in_file)
				cu->cu_type &= STDIN_MASK;
			cu->cu_type |= STDIN_REDIRECT;

			token = strtok_r(NULL, delim, &savep);

		       	if(token == NULL)
				cu->cu_type &= STDIN_MASK;
			else 
				cu->cu_in_file = token;

		}else if(token[0] == '<' && len > 1)
		{
			if(token[1] == '<')
			{
				if(cu->cu_in_file)
					cu->cu_type &= STDIN_MASK;
				cu->cu_type |= STDIN_REDIRECT_EOF;

				if(len == 2)
				{
					token = strtok_r(NULL, delim, &savep);
					if(token == NULL)
						cu->cu_type &= STDIN_MASK;
					else 
						cu->cu_in_file = token;

				}else
				{
					cu->cu_in_file = &token[2];
				}
			}else 
			{
				if(cu->cu_in_file)
					cu->cu_type &= STDIN_MASK;
				cu->cu_type |= STDIN_REDIRECT;
				cu->cu_in_file = &token[1];
			}

		}else if(token[0] == '>' && len == 1)
		{
			if(cu->cu_out_file)
				cu->cu_type &= STDOUT_MASK;
			cu->cu_type |= STDOUT_REDIRECT;

			token = strtok_r(NULL, delim, &savep);

		       	if(token == NULL)
				cu->cu_type &= STDOUT_MASK;
			else 
				cu->cu_out_file = token;

		}else if(token[0] == '>' && len > 1)
		{
			if(token[1] == '>')
			{
				if(cu->cu_out_file)
					cu->cu_type &= STDOUT_MASK;

				cu->cu_type |= STDOUT_REDIRECT_APP;

				if(len == 2)
				{
					token = strtok_r(NULL, delim, &savep);
					if(token == NULL)
						cu->cu_type &= STDOUT_MASK;
					else 
						cu->cu_out_file = token;

				}else
				{
					cu->cu_out_file = &token[2];
				}
			}else 
			{
				if(cu->cu_out_file)
					cu->cu_type &= STDOUT_MASK;

				cu->cu_type |= STDOUT_REDIRECT;
				cu->cu_out_file = &token[1];
			}
		}else if(token[0] == '|')
		{
			if(cu_len >= cu_size){

				return -1;
			}
			cu = &cu_ptr[cu_len++];
			cu_ctor(cu);

			if(len > 1)
				cu->cu_argv[cu->cu_argc++] = &token[1];
		}else
		{
			cu->cu_argv[cu->cu_argc++] = token;
		}

	}while((token = strtok_r(NULL, delim, &savep)));
	
//	printf("cu_len %d\n", cu_len);
	return cu_len;
}

int get_prompt(char *buf)
{
	struct utsname uname_info;
	struct passwd pwd;
	struct passwd *pwdret;
	char pwdbuf[512];
	uname(&uname_info);
	uid_t uid = getuid();
	
	int ret = getpwuid_r(uid, &pwd, pwdbuf, 512, &pwdret);
	if(pwdret == NULL){
		if(ret == 0){
			printf("uid %d not found\n", uid);
			return -1;
		}else {
			printf("getpwuid_r error\n");
			return -1;
		}
	}
	char cwd[128];
	getcwd(cwd, 128);

	sprintf(buf, "<%s@%s:%s> ", pwd.pw_name, uname_info.sysname, cwd);
	return 0;
}



void scan_line(char **cmdline, size_t *cmdlinesz)
{
	int bytes_read = 0;
#ifdef LINUX
	bytes_read = getline(cmdline, cmdlinesz, stdin);
	(*cmdline)[--bytes_read] = '\0';
#elif MYOS
	bytes_read = readline(cmdline, cmdlinesz, STDIN_FILENO);
	//printf("bytes_read %d\n", bytes_read);
	//(*cmdline)[bytes_read] = '\0';
	//printf("scan_line %s\n", *cmdline);
#endif
}

static inline void buildin_clear()
{
	printf("\e[2J");
}

static inline void buildin_cd(struct cmd_unit *cu)
{
	if(cu->cu_argc != 2)
	{
		printf("Usage: cd [directory]\n");
		return;
	}
	if(chdir(cu->cu_argv[1]) == -1)
	{
		printf("cd error\n");
	}
	return;
}

static inline void buildin_exit(struct cmd_unit *cu)
{
	exit(EXIT_SUCCESS);
}

int is_builtin(struct cmd_unit *cu)
{
	if(strcmp(cu->cu_argv[0], "cd") == 0)
	{
		buildin_cd(cu);
		return 0;

	}else if(strcmp(cu->cu_argv[0], "exit") == 0)
	{
		buildin_exit(cu);

	}else if(strcmp(cu->cu_argv[0], "clear") == 0)
	{
		buildin_clear();
	}
	return -1;
}

int do_redirect(struct cmd_unit *cu)
{
	if(cu->cu_out_file)
	{
		int fd = -1;
		if(cu->cu_type & STDOUT_REDIRECT)
			fd = open(cu->cu_out_file, O_RDWR);

		else if(cu->cu_type & STDOUT_REDIRECT_APP)
			//fd = open(cu->cu_out_file, O_RDWR | O_APPEND);

		if(fd == -1)
			return ERROR_FD;
		dup2(fd, STDOUT_FILENO);
	}
	if(cu->cu_in_file)
	{
		//TODO 区分 < 和 <<
		int fd = -1;
		fd = open(cu->cu_in_file, O_RDONLY);

		if(fd == -1)
			return ERROR_FD;

		dup2(fd, STDIN_FILENO);
	}
	return 0;
}

int do_pipe(struct cmd_unit *cu, int idx, int cu_len)
{
	printf("do pipe\n");
	if(idx == cu_len)
	{
		return 0;
	}

	int ret = -1;
	int fds[2];
	if(pipe(fds) == -1)
		return ERROR_PIPE;

	pid_t pid = fork();
	if(pid == -1)
		return ERROR_FORK;

	if(pid == 0)
	{
		printf("do pipe child\n");
		close(fds[0]);
		if(idx + 1 != cu_len)
			dup2(fds[1], STDOUT_FILENO);
		close(fds[1]);

		do_redirect(cu);
		execvp(cu->cu_argv[0], cu->cu_argv);
		exit(0);
	}else 
	{
		wait(NULL);
		close(fds[1]);
		dup2(fds[0], STDIN_FILENO);
		close(fds[0]);

		ret = do_pipe(++cu, ++idx, cu_len);
	}
	return ret;
}

int do_cmd(struct cmd_unit *cu_ptr, int cu_len)
{
	printf("do cmd\n");
	pid_t pid = fork();
	if(pid == -1)
		return ERROR_FORK;

	if(pid == 0)
	{
		int infd = dup(STDIN_FILENO);
		int outfd = dup(STDOUT_FILENO);
	
		int ret = do_pipe(cu_ptr, 0, cu_len);

		dup2(infd, STDIN_FILENO);
		dup2(outfd, STDOUT_FILENO);
		
		exit(ret);
	}else 
		wait(NULL);
	return 0;
}

int legal_cmd(struct cmd_unit *cu_ptr, int cu_len)
{
	//O(n^2)
	for(int j = 0; j < cu_len ; j++)
	{
		for(int i = 0; progs[i]; i++)
		{
			if(strcmp(progs[i], 
				cu_ptr[j].cu_argv[0]) == 0)
				break;
			if(!progs[i + 1])
				return -1;
		}
	}
	return 0;
}

int main()
{
	printf("hello this is myshell!\n");
	char *prompt = malloc(PROMPT_BUFSZ);
	if(prompt == NULL)
	{
		printf("malloc failed\n");
		return -1;
	}

	char *cmdline = malloc(CMDLINE_SZ);
	size_t cmdlinesz = CMDLINE_SZ;
	if(cmdline == NULL)
	{
		printf("malloc failed\n");
		free(prompt);
		return -1;
	}

	struct cmd_unit *cu_ptr = malloc(MAX_CMD * sizeof(struct cmd_unit));
	if(cu_ptr == NULL)
	{
		printf("malloc failed\n");
		free(prompt);
		free(cmdline);
		return -1;
	}

	while(1)
	{
		prompt[0] = '\0';
		if(get_prompt(prompt) == -1)
			//默认配置
			strcpy(prompt, "kyon@x86:~");

		printf("%s", prompt);

		scan_line(&cmdline, &cmdlinesz);
		int cu_len = parse_line(cmdline, cu_ptr, MAX_CMD);

		//printf("line %s\n", cmdline);
		//cu_print(cu_ptr);

		if(legal_cmd(cu_ptr, cu_len) == -1){

			printf("illegal command\n");
			continue;
		}
		if(is_builtin(cu_ptr) == -1)
			do_cmd(cu_ptr, cu_len);

	}
	
	free(prompt);
	free(cmdline);
	free(cu_ptr);
	return 0;
}
