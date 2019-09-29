

int main(int argc, char *argv[])
{

	printf("init proc start\n");
	
	open("/dev/tty0", O_RDWR);
	dup(0);
	dup(0);
	
	uint32_t pid = fork();
	if(pid){

		printf("I am father my pid is %d\n", getpid());
		char *file = "/home/test.txt";
		int fd = open(file, O_CREAT | O_RDONLY, S_IRUSR);
		if(fd == -1){

			printf("open file %s failed\n", file);
		}else {
			
			close(fd);
		}

		if((fd = open(file, O_RDWR)) == -1){

			printf("open file %s failed\n", file);
		}

		
	}else {
		printf("I am child my pid is %d\n", getpid());
		int delay = 10000;
		while(delay--);
	}
	
	while(1);

}
