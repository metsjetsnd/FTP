//Shane Brosnan (sbrosna1), Jospeh Spencer (jspence5), Tommy Lynch (client code)
//server code
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <time.h>

#define MAX_PENDING 5
#define MAX_LINE 4096

void download(int new_s){
  int len;
  FILE *fp;
  short int buf_len;
  //receive length of filename from client
  if ((len = recv(new_s, &buf_len, sizeof(buf_len), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf_len = ntohs(buf_len);
  char buf[buf_len];
  //receive filename from client
  if ((len = recv(new_s, buf, buf_len, 0)) == -1){
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
    size_t read_bytes, sent_bytes;
    char file_buf[MAX_LINE];
    int count = 0, total_bytes = 0;
    //while end of file has not been reached, send content of file
    while (1){
      int send_len = fread(file_buf, 1,  MAX_LINE, fp);
      if ((sent_bytes = send(new_s, file_buf, send_len, 0)) < 0){
        perror("Send error\n");
        exit(1);
      }
      total_bytes += sent_bytes;
      //break if whole file has been sent
      if (total_bytes >= size)
        break;
      bzero((char*)&file_buf, sizeof(file_buf));
    }
    close(fp);
  } else {
    int neg_int = -1;
    neg_int = htonl(neg_int);
    if (send(new_s, &neg_int, sizeof(neg_int), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    close(fp);
  } 
}

void upload(int new_s){
  char buf[MAX_LINE], ack_buf[5] = "Y";
  short int buf_len;
  struct timeval start, end;
  //receive length of filename from client
  if (recv(new_s, &buf_len, sizeof(buf_len), 0) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf_len = ntohs(buf_len);
  char filename[buf_len];
  int len;
  //receive filename from client
  if ((len = recv(new_s, filename, buf_len, 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  filename[buf_len - 1] = '\0';
  //send ack
  if (send(new_s, ack_buf, strlen(ack_buf) + 1, 0) < 0){
    perror("Send error\n");
    exit(1);
  }
  //receive file size
  int file_size, temp;
  if (recv(new_s, &file_size, sizeof(file_size), 0) == -1){
    perror("Receive error\n");
    exit(1);
  }
  file_size = ntohl(file_size);
  if (file_size < 0){
    printf("File does not exist on server\n");
  } else {
    FILE *fp;
    //create file with filename
    if ((fp = fopen(filename, "w")) == NULL){
      perror("Error creating file");
      exit(1);
    }
    int received_bytes, total_bytes = 0;
    char recv_buf[MAX_LINE];
    gettimeofday(&start, NULL);
    while (1){
      //receive file content from server
      if ((received_bytes = recv(new_s, recv_buf, sizeof(recv_buf), 0)) < 0){
        perror("Receive error\n");
        exit(1);
      }
      int i;
      //print received bytes to file
      for (i = 0; i < received_bytes; i++){
        fprintf(fp, "%c", recv_buf[i]);
      }
      fflush(fp);
      total_bytes += received_bytes;
      //if all file content has been receieved, break out of loop
      if (total_bytes >= file_size)
        break;
      bzero((char*)&recv_buf, sizeof(recv_buf));
    }
    gettimeofday(&end, NULL);
    int time_diff = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
    time_diff = htonl(time_diff);
    //send time for calc of throughput
    if (send(new_s, &time_diff, sizeof(time_diff), 0) < 0){
      perror("Send error\n");
      exit(1);
    }
    close(fp);
  }
}

void delete_file(int new_s){
  int len;
  FILE *fp;
  short int buf_len;
  //receive length of filename from client
  if ((len = recv(new_s, &buf_len, sizeof(buf_len), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf_len = ntohs(buf_len);
  char buf[buf_len];
  //receive filename from client
  if ((len = recv(new_s, buf, buf_len, 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf[buf_len - 1] = '\0';
  fp = fopen(buf, "r");
  int confirm;
  if (fp){ 
    confirm = 1;
  } else {
    confirm = -1;
  }
  int n_confirm = htonl(confirm);
  //send confirm of file to client
  if (send(new_s, &n_confirm, sizeof(n_confirm), 0) == -1){
    perror("Send error\n");
    exit(1);
  }
  if (confirm < 0)
    return;
  char should_delete[MAX_LINE];
  //receive Y/N confirmation
  if (recv(new_s, should_delete, sizeof(should_delete), 0) < 0){
    perror("Receive error\n");
    exit(1);
  }
  if (strncmp(should_delete, "Y", 1))
    return;
  int rem = remove(buf);
  if (rem < 0){
    char n[MAX_LINE] = "N";
    if (send(new_s, n, sizeof(n), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
  } else {
    char y[MAX_LINE] = "Y";
    if (send(new_s, y, sizeof(y), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
  }
}

void list(int new_s){
  FILE *fp, *fp2;
  char buf[MAX_LINE];
  fp = popen("ls -l", "r");
  fp2 = fopen("/tmp/compnettempfile", "w+");
  //write output of popen to file in temp dir
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL){
    fprintf(fp2, "%s", buf);
  }
  close(fp);
  if (fp2){
    rewind(fp2);
    fseek(fp2, 0L, SEEK_END);
    int size = ftell(fp2);
    int n_size = htonl(size);
    //send size of file to client
    if (send(new_s, &n_size, sizeof(n_size), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    rewind(fp2);
    size_t read_bytes, sent_bytes;
    char file_buf[MAX_LINE];
    int count = 0, total_bytes = 0;
    //while end of file has not been reached, send content of file
    while (1){
      int send_len = fread(file_buf, 1,  MAX_LINE, fp2);
      if ((sent_bytes = send(new_s, file_buf, send_len, 0)) < 0){
        perror("Send error\n");
        exit(1);
      }
      total_bytes += sent_bytes;
      //break if whole file has been sent
      if (total_bytes >= size)
        break;
      bzero((char*)&file_buf, sizeof(file_buf));
    }
    close(fp2);
  } else {
    int neg_int = -1;
    neg_int = htonl(neg_int);
    if (send(new_s, &neg_int, sizeof(neg_int), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    close(fp2);
  } 
}

void make_directory(int new_s){
  int len;
  short int buf_len;
  //receive length of filename from client
  if ((len = recv(new_s, &buf_len, sizeof(buf_len), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf_len = ntohs(buf_len);
  char buf[buf_len];
  //receive filename from client
  if ((len = recv(new_s, buf, buf_len, 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf[buf_len - 1] = '\0';
  DIR* dir = opendir(buf);
  int confirm;
  if (!dir){
    int success = mkdir(buf, 0700);
    if (success == -1)
      confirm = -1;
    else
      confirm = 1;
  } else {
    confirm = -2;
  }
  confirm = htonl(confirm);
  if (send(new_s, &confirm, sizeof(confirm), 0) == -1){
    perror("Send error\n");
    exit(1);
  }
}

void remove_directory(int new_s){
  int len;
  DIR *dir;
  short int buf_len;
  //receive length of filename from client
  if ((len = recv(new_s, &buf_len, sizeof(buf_len), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf_len = ntohs(buf_len);
  char buf[buf_len];
  //receive filename from client
  if ((len = recv(new_s, buf, buf_len, 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf[buf_len - 1] = '\0';
  dir = opendir(buf);
  int confirm;
  if (dir){ 
    confirm = 1;
  } else {
    confirm = -1;
  }
  int n_confirm = htonl(confirm);
  //send confirm of file to client
  if (send(new_s, &n_confirm, sizeof(n_confirm), 0) == -1){
    perror("Send error\n");
    exit(1);
  }
  if (confirm < 0)
    return;
  
  char temp[MAX_LINE], should_delete[MAX_LINE];
  //receive Y/N confirmation
  if (recv(new_s, should_delete, sizeof(should_delete), 0) < 0){
    perror("Receive error\n");
    exit(1);
  }
  should_delete[1] = '\0';
  if (strncmp(should_delete, "Y", 1))
    return;
  struct dirent *d;
  int num_entries = 0, rem;
  while ((d = readdir(dir)) != NULL)
    num_entries++;
  //if diretory is empty
  printf("%d\n", num_entries);
  if (num_entries == 2)
    rem = rmdir(buf);
  if (num_entries > 2 || rem < 0){
    char n[MAX_LINE] = "N";
    if (send(new_s, n, sizeof(n), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
  } else {
    char y[MAX_LINE] = "Y";
    if (send(new_s, y, sizeof(y), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
  }
}

void change_directory(int new_s){
  int len;
  short int buf_len;
  //receive length of filename from client
  if ((len = recv(new_s, &buf_len, sizeof(buf_len), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf_len = ntohs(buf_len);
  char buf[buf_len];
  //receive filename from client
  if ((len = recv(new_s, buf, buf_len, 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  buf[buf_len - 1] = '\0';
  DIR* dir = opendir(buf);
  int confirm;
  if (dir){
    int success = chdir(buf);
    if (success == -1)
      confirm = -1;
    else
      confirm = 1;
  } else {
    confirm = -2;
  }
  confirm = htonl(confirm);
  if (send(new_s, &confirm, sizeof(confirm), 0) == -1){
    perror("Send error\n");
    exit(1);
  }
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
        upload(new_s);
      if (!strncmp(buf, "DELF", 4))
        delete_file(new_s);
      if (!strncmp(buf, "LIST", 4))
        list(new_s);
      if (!strncmp(buf, "MDIR", 4))
        make_directory(new_s);
      if (!strncmp(buf, "RDIR", 4))
        remove_directory(new_s);
      if (!strncmp(buf, "CDIR", 4))
        change_directory(new_s);
      bzero((char*)&buf, sizeof(buf));
    }
    //change back to server home directory
    chdir("/afs/nd.edu/user10/sbrosna1/compnet/pa3/server");
    close(new_s);
  }
  close(s);
}
