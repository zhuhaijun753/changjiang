#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/syslog.h>

int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop,unsigned int cflag)
{
	struct termios newtio, oldtio;
	if(tcgetattr(fd, &oldtio) != 0)
	{
		printf("setupserial 1\n");
		printf("errno=%d\n", errno);
		//printf("errno=%s\n", strerror(errno));
		return -1;
	}
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |=CLOCAL | CREAD |cflag;
	//newtio.c_cflag &= ~CSIZE;
	switch(nBits)
	{
		case 7:
			newtio.c_cflag |=CS7;
			break;
		case 8:
			newtio.c_cflag |=CS8;
			break;
	}
	switch(nEvent)
	{
		case 'O':
			newtio.c_cflag |=PARENB;
			newtio.c_cflag |=PARODD;
			newtio.c_iflag |=(INPCK);
			break;
		case 'E':
			newtio.c_iflag |=(INPCK);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~ PARODD;
			break;
		case 'N':
			newtio.c_cflag &= ~PARENB;
			break;
	}
	switch(nSpeed)
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 19200:
			cfsetispeed(&newtio, B19200);
			cfsetospeed(&newtio, B19200);
			break;
		case 38400:
			cfsetispeed(&newtio, B38400);
			cfsetospeed(&newtio, B38400);
			break;
		case 57600:
			cfsetispeed(&newtio, B57600);
			cfsetospeed(&newtio, B57600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		case 230400:
			cfsetispeed(&newtio, B230400);
			cfsetospeed(&newtio, B230400);
			break;
		case 460800:
			cfsetispeed(&newtio, B460800);
			cfsetospeed(&newtio, B460800);
			break;
		default:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
	}
	if( nStop == 1)
		newtio.c_cflag &= ~CSTOPB;
	else if(nStop == 2)
		newtio.c_cflag |=CSTOPB;

	newtio.c_cc[VTIME] = 30;
	newtio.c_cc[VMIN] = 1;
	tcflush(fd, TCIFLUSH);
	tcflush(fd, TCOFLUSH);
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		printf("com set error\n");
		return -1;
	}

	return 0;
}

//int main(int argc, char *argv[])
int uart_debug()
{
	int fd;
	fd = open("/dev/ttyHS1", O_WRONLY|O_NONBLOCK , 0666);
	if (fd < 0)
	    return fd;
	set_opt(fd, 115200, 8, 'N', 1,0);
	// redirect stdout and stderr to the log file
	dup2(fd, 1);
	dup2(fd, 2);
	fprintf(stderr,"-error log test--starting (pid %d) ---\n", getpid());

	fprintf(stdout,"stdout log test\n");
	printf("printf log test\n");

	openlog ("logtest", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1); 	
    syslog (LOG_NOTICE, "syslog test");

	close(fd);
	return 0;
}