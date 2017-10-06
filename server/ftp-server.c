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
  //receive length of filename from client
  if ((len = recv(new_s, &bufLen, sizeof(bufLen), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  bufLen = ntohs(bufLen);
  char buf[bufLen];
  //receive filename from client
  if ((len = recv(new_s, buf, sizeof(buf), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf[strlen(buf) - 1] = '\0';
  fp = fopen(buf, "r");
  if (fp){
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    int n_size = htonl(size);
    //send size of file to client
    if (send(new_s, &n_size, sizeof(n_size), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    rewind(fp);
    printf("%d\n", ftell(fp));
    size_t read_bytes, sent_bytes;
    char fileBuf[MAX_LINE];
    int count = 0, total_bytes = 0;
    //while end of file has not been reached, send content of file
    while (1){
      printf("%d\n", total_bytes);
      fgets(fileBuf, MAX_LINE, fp);
      if ((sent_bytes = send(new_s, fileBuf, strlen(fileBuf) + 1, 0)) < 0){
        perror("Send error\n");
        exit(1);
      }
      total_bytes += sent_bytes;
      //break if whole file has been sent
      if (total_bytes >= size)
        break;
      bzero((char*)&fileBuf, sizeof(fileBuf));
    }
    close(fp);
  } else {
    int negInt = -1;
    negInt = htonl(negInt);
    if (send(new_s, &negInt, sizeof(negInt), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    close(fp);
  } 
}

void upload(int s){
  char buf[MAX_LINE];
  printf("Enter file to download: ");
  fgets(buf, sizeof(buf), stdin);
  short int len = htons(strlen(buf));
  //send size of file name
  if (send(s, &len, sizeof(len), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //send file name
  if (send(s, buf, sizeof(buf), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //receive file size
  int file_size, l;
  if ((l = recv(s, &file_size, sizeof(file_size), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  file_size = ntohl(file_size);
  if (file_size < 0){
    printf("File does not exist on server\n");
  } else {
    FILE *fp;
    buf[strlen(buf) - 1] = '\0';
    //create file with filename
    if ((fp = fopen(buf, "a")) == NULL){
      perror("Error creating file");
      exit(1);
    }
    int received_bytes, total_bytes = 0;
    char recv_buf[MAX_LINE];
    while (1){
      //receive file content from server
      if ((received_bytes = recv(s, recv_buf, sizeof(recv_buf), 0)) < 0){
        perror("Receive error\n");
        exit(1);
      }
      int i;
      for (i = 0; i < received_bytes; i++){
        //if (i > 0 && recv_buf[i] == '\0' && recv_buf[i - 1] == '\n')
          //continue;
        //print byte to file
        fprintf(fp, "%c", recv_buf[i]);
      } 
      total_bytes += received_bytes;
      //if all file content has been receieved, break out of loop
      if (total_bytes >= file_size)
        break;
      bzero((char*)&recv_buf, sizeof(recv_buf));
    }
    printf("%s succesfully downloaded from server\n", buf);
    close(fp);
  }
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
  
  if ((listen(s, 1)) < 0){
    perror("ftp-server: listen\n");
    exit(1);
  }
  
  while (1){
    if ((new_s = accept(s, (struct sockaddr*)&sin, &len)) < 0){
      perror("ftp-server: accept\n");
      exit(1);
    }

    while (1){
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
