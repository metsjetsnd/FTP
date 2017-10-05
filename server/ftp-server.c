//Shane Brosnan
//sbrosna1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_PENDING 5
#define MAX_LINE 4096

void download(int new_s){
  int len;
  FILE *fp;
  short int bufLen;
  if ((len = recv(new_s, &bufLen, sizeof(bufLen), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  bufLen = ntohs(bufLen);
  char buf[bufLen];
  if ((len = recv(new_s, buf, sizeof(buf), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf[strlen(buf) - 1] = '\0';
  printf("filename: %s\n", buf);
  fp = fopen(buf, "r");
  if (fp){
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    printf("%d\n", size);
    size = htonl(size);
    if (send(new_s, &size, sizeof(size), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    printf("HERE\n");
    fseek(fp, 0, SEEK_SET);
    size_t read_bytes, sent_bytes;
    char fileBuf[MAX_LINE];
    while (!feof(fp)){
      fgets(fileBuf, MAX_LINE, fp);
      printf("%s\n", fileBuf);
      if ((sent_bytes = send(new_s, fileBuf, strlen(fileBuf), 0)) < 0){
        perror("Send error\n");
        exit(1);
      }
    }
    close(fp);
  } else {
    printf("File not found\n");
    int negInt = -1;
    negInt = htonl(negInt);
    if (send(new_s, &negInt, sizeof(negInt), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
  } 
}

void upload(){

}

void delete_file(){

}

void list(){

}

void make_directory(){

}

void remove_directory(){

}

void change_directory(){
  
}

int main(int argc, char* argv[]){
  printf("HHHH\n");
  struct sockaddr_in sin, client_addr;
  char buf[MAX_LINE], outBuf[MAX_LINE];
  int len, addr_len, s, new_s, opt = 1;
  struct timeval t1;

  if (argc != 2){
    fprintf(stderr, "usage: ftp-server port\n");
  }

  bzero((char*)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons((atoi(argv[1])));
  printf("ijdoiw]\n");
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    perror("ftp-server: socket\n");
    exit(1);
  }

  if ((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int))) < 0){
    perror("ftp-server: setsockopt\n");
    exit(1);
  }

  if ((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0){
    perror("ftp-server: bind\n");
    exit(1);
  }
  printf("djwoejfow\n");
  if ((listen(s, 1)) < 0){
    perror("ftp-server: listen\n");
    exit(1);
  }
  printf("kdjwk\n");
  while (1){
    if ((new_s = accept(s, (struct sockaddr*)&sin, &len)) < 0){
      perror("ftp-server: accept\n");
      exit(1);
    }
    printf("lkjdwoieq\n");

    while (1){
      printf("djwpoej\n");
      if ((len = recv(new_s, buf, sizeof(buf), 0)) == -1){
        perror("Receive error\n");
        exit(1);
      }
      if (!len || !strncmp(buf, "QUIT", 4))
        break;
      if (!strncmp(buf, "DWLD", 4))
        download(new_s);
      if (!strncmp(buf, "UPLD", 4))
        upload();
      if (!strncmp(buf, "DELF", 4))
        delete_file();
      if (!strncmp(buf, "LIST", 4))
        list();
      if (!strncmp(buf, "MDIR", 4))
        make_directory();
      if (!strncmp(buf, "RDIR", 4))
        remove_directory();
      if (!strncmp(buf, "CDIR", 4))
        change_directory();
      bzero((char*)&buf, sizeof(buf));
    }
  }
  close(s);
}
