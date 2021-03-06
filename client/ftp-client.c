//Shane Brosnan (sbrosna1), Joseph Spencer (jspence5), Tommy Lynch (tlynch)
//Client code
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#define MAX_LINE 4096

void download(int s){
  char buf[MAX_LINE];
  struct timeval start, end;
  printf("Enter file to download: ");
  fgets(buf, sizeof(buf), stdin);
  short int len = htons(strlen(buf));
  //send size of file name
  if (send(s, &len, sizeof(len), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //send file name
  if (send(s, buf, strlen(buf), 0) == -1){
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
    gettimeofday(&start, NULL);
    while (1){
      //receive file content from server
      if ((received_bytes = recv(s, recv_buf, sizeof(recv_buf), 0)) < 0){
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
    double sec = ((double) time_diff) / 1000000.0;
    double throughput = (((double) file_size) / 1000000.0) / sec;
    printf("Download Successful\n%d bytes in %f s: %fMB/s\n", file_size, sec, throughput);
    close(fp);
  }
}

void upload(int s){
  int len;
  FILE *fp;
  short int buf_len;
  char buf[MAX_LINE];
  printf("Enter file to upload: ");
  fgets(buf, sizeof(buf), stdin);
  short int fn_len = htons(strlen(buf));
  //send size of file name
  if (send(s, &fn_len, sizeof(fn_len), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //send file name
  if (send(s, buf, strlen(buf), 0) == -1){
    perror("Client send error\n");
    exit(1);
  } 
  char ack_buf[MAX_LINE];
  //receive acknowledgement
  if (recv(s, ack_buf, sizeof(ack_buf), 0) == -1){
    perror("Receive error\n");
    exit (1);
  }
  if (strncmp("Y", ack_buf, 1)){
    printf("ACK Error\n");
    return;
  }
  buf[strlen(buf) - 1] = '\0';
  fp = fopen(buf, "r");
  if (fp){
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    int n_size = htonl(size);
    //send size of file to server
    if (send(s, &n_size, sizeof(n_size), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    rewind(fp);
    size_t read_bytes, sent_bytes;
    char file_buf[MAX_LINE];
    int count = 0, total_bytes = 0;
    //while end of file has not been reached, send content of file
    while (1){
      int send_len = fread(file_buf, 1, MAX_LINE, fp);
      if ((sent_bytes = send(s, file_buf, send_len, 0)) < 0){
        perror("Send error\n");
        exit(1);
      }
      total_bytes += sent_bytes;
      //break if whole file has been sent
      if (total_bytes >= size)
        break;
      bzero((char*)&file_buf, sizeof(file_buf));
    }
    int time_diff;
    //receive time for throughput calculation
    if (recv(s, &time_diff, sizeof(time_diff), 0) < 0){
      perror("Receive error\n");
      exit(1);
    }
    time_diff = ntohl(time_diff);
    double sec = ((double) time_diff) / 1000000.0;
    double throughput = (((double) size) / 1000000.0) / sec;
    printf("Upload Successful\n%d bytes in %f s: %fMB/s\n", size, sec, throughput);
    close(fp);
  } else {
    printf("File not found\n");
  } 
}

void delete_file(int s){
  char buf[MAX_LINE];
  printf("Enter file to delete: ");
  fgets(buf, sizeof(buf), stdin);
  short int len = htons(strlen(buf));
  //send size of file name
  if (send(s, &len, sizeof(len), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //send file name
  if (send(s, buf, strlen(buf), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  int confirm, l;
  //receive confirmation that file exists
  if ((l = recv(s, &confirm, sizeof(confirm), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  confirm = ntohl(confirm);
  if (confirm > 0){
    printf("Are you sure you want to delete the file? (Y/N): ");
    bzero((char*)&buf, sizeof(buf));
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';
    //send confirmation
    if (send(s, buf, strlen(buf) + 1, 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    if (strncmp(buf, "Y", 1)){
      printf("Delete file abandoned\n");
    } else {
      bzero((char*)&buf, sizeof(buf));
      if (recv(s, buf, sizeof(buf), 0) == -1){
        perror("Receieve error\n");
        exit(1);
      }
      if (strncmp(buf, "Y", 1))
        printf("File delete failed\n");
      else
        printf("File successfully deleted\n");
    }
  } else {
    printf("File does not exist on server\n");
  }
}

void list(int s){
  char buf[MAX_LINE];
  //receive file size
  int file_size, l;
  if ((l = recv(s, &file_size, sizeof(file_size), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  file_size = ntohl(file_size);
  if (file_size < 0){
    printf("List failed\n");
  } else {
    int received_bytes, total_bytes = 0;
    char recv_buf[MAX_LINE];
    while (1){
      //receive file content from server
      if ((received_bytes = recv(s, recv_buf, sizeof(recv_buf), 0)) < 0){
        perror("Receive error\n");
        exit(1);
      }
      int i;
      //print received bytes to file
      for (i = 0; i < received_bytes; i++){
        printf("%c", recv_buf[i]);
      }
      total_bytes += received_bytes;
      //if all file content has been receieved, break out of loop
      if (total_bytes >= file_size)
        break;
      bzero((char*)&recv_buf, sizeof(recv_buf));
    }
  }
}

void make_directory(int s){
  char buf[MAX_LINE];
  printf("Enter name of directory: ");
  fgets(buf, sizeof(buf), stdin);
  short int len = htons(strlen(buf));
  //send size of file name
  if (send(s, &len, sizeof(len), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //send file name
  if (send(s, buf, strlen(buf), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  int confirm;
  //receive status from server
  if (recv(s, &confirm, sizeof(confirm), 0) == -1){
    perror("Receive error\n");
    exit(1);
  }
  confirm = ntohl(confirm);
  if (confirm == -1){
    printf("Error creating directory\n");
  } else if (confirm == -2){
    printf("Directory already exists on server\n");
  } else {
    printf("The directory was successfully created\n");
  }
}

void remove_directory(int s){
  char buf[MAX_LINE];
  printf("Enter directory to delete: ");
  fgets(buf, sizeof(buf), stdin);
  short int len = htons(strlen(buf));
  //send size of file name
  if (send(s, &len, sizeof(len), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //send file name
  if (send(s, buf, strlen(buf), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  int confirm, l;
  //receieve from server whether directory exists on server
  if ((l = recv(s, &confirm, sizeof(confirm), 0)) == -1){
    perror("Receive error\n");
    exit(1);
  }
  confirm = ntohl(confirm);
  if (confirm > 0){
    printf("Are you sure you want to delete directory? (Y/N): ");
    bzero((char*)&buf, sizeof(buf));
    fgets(buf, sizeof(buf), stdin);
    if (send(s, buf, strlen(buf), 0) == -1){
      perror("Send error\n");
      exit(1);
    }
    if (strncmp(buf, "Y", 1)){
      printf("Delete directory abandoned\n");
    } else {
      bzero((char*)&buf, sizeof(buf));
      //receive confirmation that dir was deleted
      if (recv(s, buf, sizeof(buf), 0) == -1){
        perror("Receieve error\n");
        exit(1);
      }
      if (strncmp(buf, "Y", 1))
        printf("Directory delete failed\n");
      else
        printf("Directory successfully deleted\n");
    }
  } else {
    printf("Directory does not exist on server\n");
  }
}

void change_directory(int s){
  char buf[MAX_LINE];
  printf("Enter name of directory: ");
  fgets(buf, sizeof(buf), stdin);
  short int len = htons(strlen(buf));
  //send size of file name
  if (send(s, &len, sizeof(len), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  //send file name
  if (send(s, buf, strlen(buf), 0) == -1){
    perror("Client send error\n");
    exit(1);
  }
  int confirm;
  //receive status from server
  if (recv(s, &confirm, sizeof(confirm), 0) == -1){
    perror("Receive error\n");
    exit(1);
  }
  confirm = ntohl(confirm);
  if (confirm == -1){
    printf("Error changing directory\n");
  } else if (confirm == -2){
    printf("Directory does not exist on server\n");
  } else {
    printf("Directory change successful\n");
  }
}

int main(int argc, char * argv[]){
  FILE *fp;
  struct hostent *hp;
  struct sockaddr_in sin, server_addr;
  char *host, *port, *text;
  char in_buf[MAX_LINE], en_buf[MAX_LINE + 1], key_buf[MAX_LINE + 1], buf[MAX_LINE + 1], initial[100] = "DWLD";
  int s, len, addr_len;
  
  if (argc == 3){
    host = argv[1];
    port = argv[2];
  }
  else {
    fprintf(stderr, "usage: ftp-client host port\n");
    exit(1);
  }

  hp = gethostbyname(host);
  if (!hp){
    fprintf(stderr, "ftp-client: unknown host: %s\n", host);
    exit(1);
  }
  bzero((char*)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  bcopy(hp->h_addr, (char*)&sin.sin_addr, hp->h_length);
  sin.sin_port = htons(atoi(port));

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    perror("ftp-client: socket\n");
    exit(1);
  }

  if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0){
    perror("ftp-client: connect\n");
    close(s);
    exit(1);
  }
  
  while (1){
    printf("Enter Command: ");
    fgets(in_buf, sizeof(in_buf), stdin);
    len = strlen(in_buf) + 1;
    if (send(s, in_buf, len, 0) == -1){
      perror("Client initial send error\n");
      exit(1);
    }
    if (!strncmp(in_buf, "QUIT", 4))
      break;
    else if (!strncmp(in_buf, "DWLD", 4))
      download(s);
    else if (!strncmp(in_buf, "UPLD", 4))
      upload(s);
    else if (!strncmp(in_buf, "DELF", 4))
      delete_file(s);
    else if (!strncmp(in_buf, "LIST", 4))
      list(s);
    else if (!strncmp(in_buf, "MDIR", 4))
      make_directory(s);
    else if (!strncmp(in_buf, "RDIR", 4))
      remove_directory(s);
    else if (!strncmp(in_buf, "CDIR", 4))
      change_directory(s);
    else
      printf("Bad Command\n");
  }
  
  close(s);
}
