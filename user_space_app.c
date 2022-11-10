#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAX_SIZE 256

int main()
{
	char recv_buf[MAX_SIZE];
	char *buf = "Hello from user space";
	printf("Opening a device\n");
	int fd = open("/dev/char_dev",O_RDWR);
	if(fd < 0)
	{
		printf("Cannot open a device\n");
		return -1;
	}
	if(write(fd,buf,strlen(buf))<0)
	{
		printf("Write operation failed\n");
		return -2;
	}
	if(lseek(fd,0,SEEK_SET) < 0)
	{
		printf("Seek operation failed\n");
		return -3;
	}
	if(read(fd,recv_buf,MAX_SIZE)<0)
	{
		printf("Read operation failed\n");
		return -4;
	}
	printf("Received string: %s\n",recv_buf);
	return 0;
}


