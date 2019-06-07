/*************************************************************************
    > File Name: raspiloader.c
    > Author: gyx
    > Mail: 3224592879@qq.com
    > Created Time: 2019年04月28日 星期日 16时57分39秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <stdint.h>
#include <termios.h>
#include <stdbool.h>



#define BUF_SIZE 65536

void do_exit(int fd, int res) {
    // close FD
    if (fd != -1) close(fd);
    exit(res);
}

// open serial connection
int open_serial(const char *dev) {
    
    struct termios termios;
    int fd = open(dev, O_RDWR | O_NOCTTY);
    printf("fd=%d\r\n",fd);
    if (fd == -1) 
    {
    // failed to open
        return -1;
    }
    // must be a tty
    if (!isatty(fd))
    {
        fprintf(stderr, "%s is not a tty\n", dev);
        do_exit(fd, EXIT_FAILURE);
    }

    // Get the attributes.
    if(tcgetattr(fd, &termios) == -1)
    {
        perror("Failed to get attributes of device");
        do_exit(fd, EXIT_FAILURE);
    }
    // So, we poll.
    termios.c_cc[VTIME] = 0;
    termios.c_cc[VMIN] = 0;

    // 8N1 mode, no input/output/line processing masks.
    termios.c_iflag = 0;
    termios.c_oflag = 0;
    termios.c_cflag = CS8 | CREAD | CLOCAL;
    termios.c_lflag = 0;

    // Set the baud rate.
    if((cfsetispeed(&termios, B115200) < 0) ||
       (cfsetospeed(&termios, B115200) < 0))
    {
        perror("Failed to set baud-rate");
        do_exit(fd, EXIT_FAILURE);
    }

    // Write the attributes.
    if (tcsetattr(fd, TCSAFLUSH, &termios) == -1) {
        perror("tcsetattr()");
        do_exit(fd, EXIT_FAILURE);
    }
    return fd;
}

// send kernel to rpi
void send_kernel(int fd, const char *file) {
    int file_fd;
    off_t off;
    uint32_t size;
    ssize_t pos;
    char *p;
    bool done = false;

    // Open file
    if ((file_fd = open(file, O_RDONLY)) == -1) {
        perror(file);
        do_exit(fd, EXIT_FAILURE);
    }

    // Get kernel size
    off = lseek(file_fd, 0L, SEEK_END);
    if (off > 0x200000) {
        fprintf(stderr, "kernel too big\n");
        do_exit(fd, EXIT_FAILURE);
    }
    size = htole32(off);
    lseek(file_fd, 0L, SEEK_SET);

    printf("### sending kernel %s [%zu byte]\n", file, (size_t)off);

    // send kernel size to RPi
    p = (uint8_t*)&size;
    pos = 0;   
    while(pos < 4) 
    {
        ssize_t len = write(fd, &p[pos], 4 - pos);
        	tcdrain(fd);
        if (len == -1) {
            perror("write()");
            do_exit(fd, EXIT_FAILURE);
        }
        pos += len;
    }
    //check size
    uint8_t sizee[4]={0};
    pos=0;
    while(pos<4){
    int nn=read(fd, sizee, 4-pos);
        pos+=nn;
    }
    uint32_t size_check=sizee[0]<<24|sizee[1]<<16|sizee[2]<<8|sizee[3];
    if(size_check!=size)
    {
        perror("size check error");
        do_exit(fd, EXIT_FAILURE);
    }
    while(!done)
    {
        char buf[BUF_SIZE];
        ssize_t pos = 0;
        ssize_t len = read(file_fd, buf, BUF_SIZE);
        switch(len) {
        case -1:
            perror("read()");
            do_exit(fd, EXIT_FAILURE);
        case 0:
            done = true;
        }
        
        while(len > 0) 
        {
            ssize_t len2 = write(fd, &buf[pos], len);
            
            tcdrain(fd);        
            
            if (len2 == -1) {
            perror("write()");
            do_exit(fd, EXIT_FAILURE);
            }
            len -= len2;
            pos += len2;
        }
    }
    fprintf(stderr, "### finished sending\n");

    return;
}

int main(int argc, char *argv[]) {

    int fd;
    char buf[BUF_SIZE];
    fd_set rfds, wfds, efds;

    printf("raspiloader v1.0\n");

    if(argc != 3)
    {
        printf("USAGE: %s <dev> <file>\n", argv[0]);
        printf("Example: %s /dev/ttyUSB0 kernel/kernel.img\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fd=open_serial(argv[1]);
    if(fd == -1)
    {
        perror(argv[1]);
        do_exit(fd, EXIT_FAILURE);
    }
    printf("Start...\n");
    while(1)
    {
        char *head = "start";
        while(1)
        {
            char buf2[BUF_SIZE]={0};
            write(fd,head,5);
            tcdrain(fd);   
            for(int i=0;i<5;i++)
            { 
                usleep(1000);
                int n = read(fd,buf2,BUF_SIZE);
                if(n>0) break;
            }
            if(!strcmp(buf2,"OK"))
            {
	            break;
            }
        }
        printf("Download Kernel\r\n");
        send_kernel(fd, argv[2]);
        break;        
    }
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    while(1)
    {
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		FD_SET(fd, &rfds);
		int ret=select(fd+1, &rfds, &wfds, &efds, NULL);
		if(ret)
		{
			// input from the user, copy to RPi
			if (FD_ISSET(STDIN_FILENO, &rfds)) {
				ssize_t len = read(STDIN_FILENO, &buf[0], BUF_SIZE);
				ssize_t len2 = write(fd, buf, len);
			}
			if (FD_ISSET(fd, &rfds)) {
				 char buf2[BUF_SIZE];
				ssize_t len = read(fd, buf2, BUF_SIZE);
				ssize_t len2 = write(STDOUT_FILENO, buf2, len);		    
			}
		}
	}
    
    close(fd);
    return 0;
}

