#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define _POSIX_SOURCE
#define DATASIZE 1024
#define BUF_LEN 1024

struct myftph {
        uint8_t type;
        uint8_t code;
        uint16_t length;
};

void excute_stor(int sock, struct myftph *r_fhp, char *r_dp)
{
  char *data;
  char *fhp_pay;
  struct myftph *fhp;
  char buf[DATASIZE];
  int n,m;
  fhp = (struct myftph *)buf;
  char *dp;
  char pkt[sizeof(struct myftph) + DATASIZE];
  FILE *fp;
  char tmpbuf[DATASIZE];
  int fd;

  data = (char *)malloc(sizeof(struct myftph) + sizeof(uint8_t)*DATASIZE);
  fhp_pay = data + sizeof(struct myftph);
  dp = (char *)(fhp + 1);
  fhp->length = 0;

      
  fhp->type = 0x10;
  fhp->code = 0x02;
  fhp->length = 0;

  memcpy(data,buf,DATASIZE);
  printf("send %02X\n", fhp->type);
  if ((n = send(sock, data, sizeof(data), 0)) < 0) {
    perror("send in put_proc\n");
    exit(1);
  }

  if (fhp->type != 0x10)
    return;
  
  if ((n = recv(sock, data, sizeof(struct myftph)+DATASIZE, 0)) < 0) {
    perror("recieve error\n");
    exit(1);
  }
  
  if (n == 0) {
    fprintf(stderr, "connection has closed \n");
    exit(1);
  }
  memcpy(buf,data,DATASIZE);
  
  if (fhp->type != 0x20) {
    printf("recieve %02X\n", fhp->type);
    fprintf(stderr, "unexpected message \n");
    return;
  } else {
    printf("Open %s\n", r_dp);
    if ((fp = fopen(r_dp, "w")) < 0) {
      if (errno == EEXIST) {
	fhp->code = 0x01;
      } else {
	int number = errno;
        printf("%s\n", strerror(number));
	creat(r_dp,S_IWUSR);
      }
    }
    printf("%s\n", fhp_pay);
    n = fputs(fhp_pay,fp);
      if (n < 0) {
        perror("write error\n");
        return;
      }
    }
  fclose(fp);
  return;
}


void excute_retr(int sock, struct myftph *r_fhp, char *r_dp){
  char *data;
  char *ftp_pay;
  
  struct myftph *fhp;
  int n, flags;
  char buf[DATASIZE];
  int fd;
  struct stat sbuf;


  fhp = (struct myftph*)buf;
  data = (char *)malloc(sizeof(struct myftph) + sizeof(uint8_t)*DATASIZE);
  ftp_pay = data + sizeof(struct myftph);
  
  flags = 0;

  if ((fd = open(r_dp, O_RDONLY)) < 0 || stat(r_dp, &sbuf) < 0) {

    fhp->type = 0x12;
    fhp->length = 0;

    if (errno == ENOENT) {
      fhp->code = 0x00;
    } else if (errno == EACCES) {
      fhp->code = 0x01;
    } else {
      fhp->type = 0x13;
      fhp->code = 0x05;
    }
    memcpy(data, buf,sizeof(struct myftph));
    printf("send %02X\n", fhp->type);
    if ((n = send(sock, data, sizeof(data), 0)) < 0) {
      perror("send error\n");
      return;
    }

    return;
  } else if (S_ISDIR(sbuf.st_mode)) {
    fhp->type = 0x12;
    fhp->code = 0x02;
    fhp->length = 0;
    memcpy(data,buf,sizeof(struct myftph));
    printf("send %02X\n", fhp->type);
    if ((n = send(sock, data, sizeof(data), 0)) < 0) {
      perror("send error \n");
      return;
    }
    } else {
    fhp->type = 0x10;
    fhp->code = 0x01;
    fhp->length = 0;
    memcpy(data,buf,DATASIZE);
    printf("send %02X\n", fhp->type);
    if ((n = send(sock, data, sizeof(data), 0)) < 0) {
      perror("send error \n");
      return;
    }

    while (!flags) {
      fhp->type = 0x20;
      fhp->length = 0;
      n = read(fd, ftp_pay, DATASIZE);
      if (n < 0) {
	fhp->type = 0x13;
	fhp->code = 0x05;
	memcpy(data,buf,sizeof(struct myftph));
	printf("send %02X\n", fhp->type);
	if ((n = send(sock, data, sizeof(data), 0)) < 0) {
	  perror("send error \n");
	  exit(1);
	}
      }
      fhp->length += n;

      if (n == DATASIZE) {
	fhp->code = 0x01;
	flags = 0;
      } else {
	fhp->code = 0x00;
	flags = 1;
      }
      memcpy(data,buf,sizeof(struct myftph));
      printf("send %02X\n", fhp->type);
      printf("contents:%s\n", ftp_pay);
      if ((n = send(sock, data, sizeof(data)+DATASIZE, 0)) < 0) {
	perror("send error\n");
	exit(1);
      }
    }
  }
  close(fd);
  return;
}


 void child_proc(int sock)
{
        struct myftph* ftp;
	char *data;
	char *ftp_pay;
	char buf[DATASIZE];
	ftp = (struct myftph *)buf;
        char* dp;
        int n;
	
        

	data = (char *)malloc(sizeof(struct myftph) + sizeof(uint8_t)*n);
        
        ftp_pay = data + sizeof(struct myftph);

        while (1) {
               

                n = 0;
                
                
                if ((n = recv(sock, data, DATASIZE, 0)) < 0) {
                        fprintf(stderr,"receive error\n");
                        exit(1);
                }
		memcpy(buf,data,DATASIZE);
                
                if (n == 0) {
                        printf("Client has closed connection\n");
                    exit(0);
            }

		if(ftp->type == 0x05){
		  printf("ftp_pay :%s\n", ftp_pay);
		  excute_retr(sock, ftp, ftp_pay);
		}
		else if(ftp->type == 0x06){
		  printf("ftp_pay :%s\n", ftp_pay);
		  excute_stor(sock, ftp, ftp_pay);
		}

	      
        }
}

int main(int argc, char *argv[])
{
	int s, wait_s;
	struct sockaddr_in c_skt;
	struct sockaddr_in s_skt;
	socklen_t sktlen;

	pid_t pid;
	int i, n;
	char pBuf[BUF_LEN];

	if (argc > 2) {
		fprintf(stderr, "Usage: myftps\n");
		exit(1);
	}

    if (argc == 2) {
		if (chdir(argv[1]) < 0) {
			fprintf(stderr, "unexpected command \n");
			exit(1);
		}
	}

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	memset(&s_skt, 0, sizeof(s_skt));
	s_skt.sin_family = AF_INET;
	s_skt.sin_addr.s_addr = htonl(INADDR_ANY);
	s_skt.sin_port = htons(10000);

	if (bind(s, (struct sockaddr *)&s_skt, sizeof(s_skt)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(s, 5) < 0) {
		perror("listen");
		exit(1);
	}

	sktlen = sizeof(c_skt);

	while (1) {
		if ((wait_s = accept(s, (struct sockaddr *)&c_skt, &sktlen)) < 0) {
			perror("accept");
			exit(1);
		}

		if ((pid = fork()) == 0) {
			printf("Client found \n");
			child_proc(wait_s);
		} else if (pid > 0) {
		  
		} else {
			fprintf(stderr, "fork error\n");
			exit(1);
		}
    }
	close(s);
	close(wait_s);
	
	return 0;
}
