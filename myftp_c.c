#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<errno.h>

#define _POSIX_SOURCE
#define DATASIZE 1024
#define BUFSIZE 1024

struct myftph{
    uint8_t type;
    uint8_t code;
    uint16_t length;
  };



void execute_put (int sock, char *com[]) {
  char *data;
  char *ftp_pay;
  struct myftph *send_header;
  FILE *fp;
  int fd;
  char buf[BUFSIZE];
  char contents[BUFSIZE];
  int n,m;
  int flag = 0;
  send_header = (struct myftph *)buf;
  
  fd = open(com[1], O_RDONLY);
    if(fd < 0){
    printf("file open error\n");
    exit(1);
  }
  if((n=read(fd,contents,BUFSIZE)) < 0){
    printf("file read error\n");
    exit(1);
  }
  printf("open %s\n", com[1]);
  data = (char *)malloc(sizeof(struct myftph) + sizeof(uint8_t)*n);
  ftp_pay = data + sizeof(struct myftph);
  send_header->type = 0x06;
  send_header->code = 0x01;
  memcpy(data,buf,sizeof(struct myftph)+sizeof(uint8_t)*n);
  memcpy(ftp_pay,com[1],strlen(com[1]));
  send(sock, data, sizeof(struct myftph)+strlen(com[1]), 0);
  printf("send %02X\n", send_header->type);
  printf("file: %s", ftp_pay);
  if (n < BUFSIZE){
    if ((m = recv(sock, data, sizeof(struct myftph), 0)) < 0) {
                perror("recieve error \n");
                exit(1);
        }
        if (m == 0) {
                fprintf(stderr, "connection has closed \n");
                exit(1);
	}
	
	memcpy(buf,data,n);
	printf("recieve %02X\n", send_header->type);
	if (send_header->type == 0x10) {
	  while (!flag){
            send_header->type = 0x20;
            send_header->code = 0x00;
            send_header -> length = htons(n);
	    memcpy(data,buf,sizeof(struct myftph)+n);
	    
	    printf("%s\n", buf);
	    memcpy(ftp_pay,contents,n);
            if ((send(sock, data, sizeof(struct myftph) + n, 0))<0) {
              perror("sendto");
              exit(1);
            }
	    printf("contents:%s\n", ftp_pay);
	    printf("send %02X\n", send_header->type);
	    flag=1;
	  }
	}
  }
  else{
    
  }
}

void execute_get (int sock, char *com[]){
   char *data;   
   char *ftp_pay;
   struct myftph *get_header;
   char buf[BUFSIZE];
   
   get_header = (struct myftph *)buf;
   
   int fd;
   int n;

   data = (char *)malloc(sizeof(struct myftph) + sizeof(uint8_t)*DATASIZE);
   ftp_pay = data + sizeof(struct myftph);
 
   get_header->type = 0x05;
   get_header->code = 0;
   get_header->length = 0;
   

   if (com[1] == NULL ) {
     fprintf(stderr, "Usage: get filename \n");
     return;
   } else {
     if (strncpy(data, com[1], DATASIZE) == NULL) {
       fprintf(stderr, "Cannot analyze %s\n", com[1]);
       return;
     }
     get_header->length = strlen(data);
   }

   memcpy(data,buf,sizeof(struct myftph)+sizeof(uint8_t)*BUFSIZE);
   memcpy(ftp_pay,com[1],strlen(com[1]));
   
   if ((n = send(sock, data, sizeof(struct myftph) + sizeof(uint8_t)*BUFSIZE, 0)) < 0) {
     perror("send error \n");
     exit(1);
   }
   printf("send %02X\n", get_header->type);
   if ((n = recv(sock, data, sizeof(struct myftph) + sizeof(uint8_t)*BUFSIZE, 0)) < 0) {
     perror("recieve error \n");
     exit(1);
   }
   memcpy(buf,data,DATASIZE);
   printf("recieve %02X\n", get_header->type);
   
   if (n == 0) {
     fprintf(stderr, "connection has closed\n");
     exit(1);
   }
   if (get_header->type == 0x10) {
       
       if ((fd = open(com[1], O_WRONLY|O_CREAT|O_EXCL, 0644)) < 0) {
	 if (errno == EEXIST) {
	   fprintf(stderr, "file already exists \n");
	 } else {
	   fprintf(stderr, "open error \n");
	   fprintf(stderr, "errno: %d \n", errno);
	 }

	 while (!(get_header->type == 0x20 && get_header->code == 0x00)) {
	   if ((n = recv(sock, data, sizeof(data), 0)) < 0) {
	     perror("recieve error\n");
	   }
	   memcpy(buf,data,BUFSIZE);
	   printf("recieve %02X\n", get_header->type);
	   if (n == 0) {
	     fprintf
	       (stderr, "connection has closed\n");
	     exit(1);
	   }
	 }
	 return;
       }
     
     memcpy(buf,data,BUFSIZE);
     printf("recieve %02X\n", get_header->type);
     memset(data,0,BUFSIZE);
     while (!(get_header->type == 0x20 && get_header->code == 0x00)) {
       if ((n = recv(sock, data, sizeof(struct myftph)+n+1, 0)) < 0) {
	 perror("recieve error \n");
       }
       memcpy(buf,data,n+1);
       printf("recieve  %02X\n", get_header->type);
       printf("ftp_pay:%s\n", ftp_pay);
       if (n == 0) {
	 fprintf(stderr,"connection has closed\n");
	 exit(1);
       }       
       if (get_header->type != 0x20) {
	 perror("unexpected message \n");
	 return;
       }
       
       n = write(fd, ftp_pay, n-sizeof(struct myftph));
       if (n < 0) {
	 perror("write error\n");
	 return;
       } 
     }
   } else if (get_header->type == 0x12) {
     if (get_header->code == 0x00) {
       fprintf(stderr, "No such file \n");
     } else if (get_header->code == 0x01) {
       fprintf(stderr, "Can't access\n");
     } else if (get_header->code == 0x02) {
       fprintf(stderr, "Can't get file\n");
     } else {
       fprintf(stderr, "Unkown error\n");
     }
     return;
   } else {
     fprintf(stderr, "Error \n");
     return;
   }
   close(fd);     
   return;
}


int main(){
  
  int sock;
  struct sockaddr_in myskt;
  int s, count, datalen;
  struct sockaddr_in skt;
  char sbuf[512];
  char rbuf[512];
  char *com[5];
  char *delim;
  delim = " \n\t";
  in_port_t port = atoi("10000");
  struct in_addr ipaddr;
  

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
      perror("socket\n");
      exit(1);
    }
  memset(&myskt, 0, sizeof myskt);
  myskt.sin_family = AF_INET;
  myskt.sin_port = htons(port);
  myskt.sin_addr.s_addr = htonl(ipaddr.s_addr);
  
  if(connect(sock,(struct sockaddr *)&myskt, sizeof myskt) < 0){
    perror("connect\n");
    exit(1);
  }
  
  for(;;){
    printf("FTP$ ");
    fgets(sbuf, sizeof(sbuf)-1, stdin);
    sbuf[strlen(sbuf)] = '\0';
    char kari;
    int j=0,k=0;
    com[0] = strtok(sbuf, delim);
    for (j = 1;; j++) {
      com[j] = strtok(NULL, delim);
      if (com[j] == NULL)
	break;
                }

    
    char *ip = "127.0.0.1";

    
    if(strncmp(com[0], "get", 3) == 0){
      execute_get(sock, com);
      printf("get\n");
  
    }
    else if(!strncmp(com[0], "put", 3)){
      execute_put(sock, com);
      printf("put\n");
    }
    else if(!strncmp(com[0],"exit", 4)){
      break;
      }
    else{
      printf("%s, %s\n", com[0], com[1]);
    }
  }
}





    

    
