#include     <stdio.h>
#include     <stdlib.h> 
#include     <unistd.h>  
#include     <sys/types.h>
#include     <sys/stat.h>
#include     <fcntl.h> 
#include     <termios.h>
#include     <errno.h>
#include     <string.h>

#include "TsipParser.h"

#define PKT_LEN 300
   
int main()
{
    int fd;
    int i;
    int len;
    int n = 0;      
    unsigned char read_buf[PKT_LEN];
    unsigned char data_buf[PKT_LEN];
    struct termios opt; 
    
    fd = open("/dev/ttyS0", O_RDWR);    
    if(fd == -1)
    {
        perror("open serial 0\n");
        exit(0);
    }

    tcgetattr(fd, &opt);      
    cfsetispeed(&opt, B9600);
    cfsetospeed(&opt, B9600);
    
    if(tcsetattr(fd, TCSANOW, &opt) != 0 )
    {     
       perror("tcsetattr error");
       return -1;
    }
    opt.c_cflag &= ~(PARENB | PARODD | CRTSCTS);
	opt.c_cflag |= CREAD | CLOCAL;
	opt.c_iflag = opt.c_oflag = opt.c_lflag = (tcflag_t) 0;

opt.c_iflag &=~ (PARMRK | INPCK);
	opt.c_cflag &=~ (CSIZE | CSTOPB | PARENB | PARODD);


    opt.c_cflag |= CS8;   
    opt.c_iflag |= INPCK;
	    opt.c_cflag |= PARENB | PARODD;
 
    
   

    opt.c_cc[VTIME] = 0;
    opt.c_cc[VMIN] = 0;
    
    tcflush(fd, TCIOFLUSH);
 
    printf("configure complete\n");
    
    if(tcsetattr(fd, TCSANOW, &opt) != 0)
    {
        perror("serial error");
        return -1;
    }
    printf("start send and receive data\n");
  
    while(1)
    {    
        n = 0;
        len = 0;
        memset(&read_buf, 0,sizeof(read_buf));   
        memset(&data_buf, 0,sizeof(data_buf));
 
        while( (n = read(fd, read_buf, sizeof(read_buf))) > 0 )
        {
            for(i = len; i < (len + n); i++)
            {
                data_buf[i] = read_buf[i - len];
            }
            len += n;
        }
        data_buf[len] = '\0';
              
	/*for(i = 0; i < len; i++) {
		printf("%x ", data_buf[i]);
	}
        printf("\n");*/
 
        CTsipParser *ctp = new CTsipParser();
        ctp->ReceivePkt(data_buf, len);
        sleep(1);
    }
    return 0;
}
