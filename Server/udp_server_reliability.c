

/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */

#include "send_receive.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd;                    /* socket */
  int portno;                    /* port to listen on */
  int clientlen;                 /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp;         /* client host info */
  char buf[BUFSIZE];             /* message buf */
  char command[BUFSIZE];         // to store the command part
  char file_name[BUFSIZE];       // tostore the file name.
  char *hostaddrp;               /* dotted decimal host addr string */
  int optval;                    /* flag value for setsockopt */
  int n;                         /* message byte size */

  /*
   * check command line arguments
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /*
   * socket: create the parent socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
             sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /*
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * receiver: receive a UDP datagram from a client
     */
    bzero(command, 1024);
    bzero(file_name, 1024);
    bzero(buf, BUFSIZE);
    /*
    buf to carry the user input completely
    command to carry only the command to execute
    file_name carries the name of the file on which actions are performed.*/
    n = receiver(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr,
                 &clientlen);
    if (n < 0)
      error("ERROR in receiver");
    n = receiver(sockfd, command, BUFSIZE, 0, (struct sockaddr *)&clientaddr,
                 &clientlen);
    if (n < 0)
      error("ERROR in receiver");
    n = receiver(sockfd, file_name, BUFSIZE, 0, (struct sockaddr *)&clientaddr,
                 &clientlen);
    if (n < 0)
      error("ERROR in receiver");
    bzero(buf, BUFSIZE);
    // checks whether command to execute is get.
    if (!(strcmp(command, "get"))) {
      clientlen = sizeof(clientaddr);
      printf("Locating and getting file named %s \n", file_name);
      FILE *file_read = fopen(file_name, "rb");
      if (file_read == NULL) {
        // to send a message stating file does not exist.
        strcpy(buf, "donotexist");
        printf("The file %s requested  DOES NOT EXIST \n", file_name);
        // if file does not exist sends a message of does not exist instead of
        // file size.
        n = sender(sockfd, buf, 1024, 0, (struct sockaddr *)&clientaddr,
                   clientlen);
        if (n < 0)
          error("ERROR in sender");
        continue;
      }
      // now should send the file size to the client.
      // Seek to the end of the file to determine its size
      fseek(file_read, 0, SEEK_END);
      // Get the file size
      int file_size = ftell(file_read);
      // fclose(file_read);
      printf("File_size is determined as %d \n", file_size);
      sprintf(buf, "%d", file_size);
      // if file exists send the file_size.
      n = sender(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr,
                 clientlen);
      if (n < 0)
        error("ERROR in sender");
      // sets cursor to the start of file to read the data.
      bzero(buf, sizeof(buf));
      fseek(file_read, 0, SEEK_SET);

      // have to send file data 1024 bytes at atime.
      int cumulatives_bytes_transferred = 0;
      int byte_transfer_size = 1024;
      for (int cumulatives_bytes_transferred = 0;
           file_size >= cumulatives_bytes_transferred;
           cumulatives_bytes_transferred += byte_transfer_size) {
        bzero(buf, sizeof(buf));
        // considering last transfer case.
        if (file_size - cumulatives_bytes_transferred < 1024) {
          byte_transfer_size = file_size - cumulatives_bytes_transferred;
        }
        // TRies to send the byte_transfer_size amount of bytes.
        int bytes_sent = fread(buf, 1, byte_transfer_size, file_read);
        n = sender(sockfd, buf, bytes_sent, 0, (struct sockaddr *)&clientaddr,
                   clientlen);
        if (n < 0)
          error("ERROR in sender");
        // cumulative count increments by an amount of byte_transfer_size.
        printf("Sent %d bytes of data in total till now from %d in total\n",
               cumulatives_bytes_transferred, file_size);

        if (byte_transfer_size < 1024) {
          printf("file transfer complete from  server\n");
          break;
        }
      }
      fclose(file_read);
      printf("get command executed successfully");

    }

    // put command execution in server side.
    else if (!(strcmp(command, "put"))) {
      printf("Receiving file size\n");
      // receives the file size.
      n = receiver(sockfd, buf, 1024, 0, (struct sockaddr *)&clientaddr,
                   &clientlen);
      if (n < 0)
        error("ERROR in receiver");

      // In the case of file existing we first receive the size of the file in
      // bytes.
      int file_size = atoi(buf);
      char *endptr;
      file_size = strtol(buf, &endptr, 10);
      bzero(buf, 1024);
      printf("Receiving file %s from client  of Filesize: %d\n", file_name,
             file_size);
      FILE *file_write = fopen(file_name, "wb");
      if (file_write == NULL) {
        error("Error in opening the file to write the data that is to be "
              "recieved");
      }
      // int cumulatives_bytes_transferred=0;
      int byte_transfer_size = 1024;
      for (int cumulatives_bytes_transferred = 0;
           file_size >= cumulatives_bytes_transferred;
           cumulatives_bytes_transferred += byte_transfer_size) {
        bzero(buf, 1024);
        // considering last transfer case.
        if (file_size - cumulatives_bytes_transferred < 1024) {
          byte_transfer_size = file_size - cumulatives_bytes_transferred;
        }
        // receives data as a set of packets.
        n = receiver(sockfd, buf, 1024, 0, (struct sockaddr *)&clientaddr,
                     &clientlen);
        if (n < 0)
          error("ERROR in recvfrom");
        // cumulative count increments by an amount of byte_transfer_size.
        printf("Received %d bytes of data in total till now from %d in total\n",
               cumulatives_bytes_transferred, file_size);
        // writes to the file.
        fwrite(buf, byte_transfer_size, 1, file_write);
        if (byte_transfer_size < 1024) {
          printf("file transfer complete to client\n");
          // fclose(file_write);
          break;
        }
      }
      fclose(file_write);
    }
    // delete command .
    else if (!(strcmp(command, "delete"))) {
      clientlen = sizeof(clientaddr);
      printf("Locating and getting file named %s \n", file_name);
      FILE *file_read = fopen(file_name, "rb");
      // file to delete does not exist.
      if (file_read == NULL) {
        // to send a message stating file does not exist.
        strcpy(buf, "donotexist");
        printf("The file %s requested  DOES NOT EXIST \n", file_name);
        n = sender(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr,
                   clientlen);
        if (n < 0)
          error("ERROR in sender");
        continue;
      }
      fclose(file_read);
      // remove of file has been started to execute and if successsfully done
      // sends deleteed to client.
      if (remove(file_name) == 0) {
        strcpy(buf, "deleted");
      } else {
        perror("Error deleting file");
        printf("deleting unsuccessfull\n");
      }
      n = sender(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr,
                 clientlen);
      if (n < 0)
        error("ERROR in sender");
    }
    // ls command
    else if ((!strcmp(command, "ls"))) {
      DIR *directory;
      bzero(buf, 1024);
      struct dirent *entry;
      // opens current directory.
      directory = opendir(".");
      while ((entry = readdir(directory)) != NULL) {
        // loops through all the files in the directory.
        strncat(buf, entry->d_name, sizeof(entry->d_name));
        strncat(buf, "\n", 1); // Add newline after each entry
      }
      closedir(directory);
      // sends list of files as a single string.
      n = sender(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr,
                 clientlen);
      if (n < 0)
        error("ERROR in sender");
    }

    // exiting command
    else if (!(strcmp(command, "exit"))) {
      printf("Client is Done: Good Bye\n");
      break;
    }
    /*
     * sender: echo the input back to the client
     */
    else {
      // sends back command to the client.
      bzero(command,1024);
      //command="InvalidCommand";
      strcpy(command,"InvalidCommand-NotUnderstood\n");
      n = sender(sockfd, command, 1024, 0, (struct sockaddr *)&clientaddr,
                 clientlen);
      if (n < 0)
        error("ERROR in sender");
    }
  }
  return 0;
}
