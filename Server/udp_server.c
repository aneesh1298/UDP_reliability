

/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char command[BUFSIZE];// to store the command part
  char file_name[BUFSIZE]; // tostore the file name.
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

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
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
     bzero(command,1024);
     bzero(file_name,1024);
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
    n = recvfrom(sockfd, command, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
    n = recvfrom(sockfd, file_name, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    //hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  //sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    // hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
    //                   sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    // if (hostp == NULL)
    //   error("ERROR on gethostbyaddr");
    // hostaddrp = inet_ntoa(clientaddr.sin_addr);
    // if (hostaddrp == NULL)
    //   error("ERROR on inet_ntoa\n");
    // printf("server received datagram from %s (%s)\n", 
	  //  hostp->h_name, hostaddrp);
    //printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    //use string tokenization to capture different parts of the command
    // char *token = strtok(buf, " \t\n");
    // if (token != NULL) 
    // {
    //     strcpy(command, token); // Copy the first token to cmd
    //     token = strtok(NULL, " \t\n"); // Get the next token
    //     if (token != NULL) 
    //     {
    //         strcpy(file_name, token); // Copy the second token to fname
    //     }
    // }
    bzero(buf, BUFSIZE);
    if(!(strcmp(command,"get")))
    {
        clientlen = sizeof(clientaddr);
        printf("Locating and getting file named %s \n", file_name);
        FILE* file_read = fopen(file_name, "rb");
        if(file_read== NULL)
        {
          // to send a message stating file does not exist.
          strcpy(buf, "donotexist");
          printf("The file %s requested  DOES NOT EXIST \n", file_name);
          n = sendto(sockfd, buf, 1024, 0,(struct sockaddr *) &clientaddr, clientlen);
          if (n < 0)
            error("ERROR in sendto");
          continue;
        }
        // now should send the file size to the client.
        // Seek to the end of the file to determine its size
        fseek(file_read, 0, SEEK_END);
        // Get the file size
        int file_size= ftell(file_read);
        //fclose(file_read);
        printf("File_size is determined as %d \n",file_size);
        sprintf(buf, "%d", file_size);
        n = sendto(sockfd, buf, strlen(buf), 0,
               (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          error("ERROR in sendto");
        bzero(buf, sizeof(buf));
        fseek(file_read, 0, SEEK_SET);

        // have to send file data 1024 bytes at atime.
            int cumulatives_bytes_transferred=0;
            int byte_transfer_size=1024;
            for(int cumulatives_bytes_transferred=0; file_size>=cumulatives_bytes_transferred;cumulatives_bytes_transferred+=byte_transfer_size)
            {
              bzero(buf, sizeof(buf));
              // considering last transfer case.
              if(file_size- cumulatives_bytes_transferred < 1024)
              {
                byte_transfer_size= file_size- cumulatives_bytes_transferred;
              }
              int bytes_sent=fread(buf, 1, byte_transfer_size, file_read);
              n = sendto(sockfd, buf, bytes_sent, 0, (struct sockaddr *)&clientaddr, clientlen);
              if (n < 0)
                error("ERROR in sendto");
              // cumulative count increments by an amount of byte_transfer_size.
              printf("Sent %d bytes of data in total till now from %d in total\n",cumulatives_bytes_transferred,file_size);
              
              if(byte_transfer_size<1024)
              {
                printf("file transfer complete from  server\n");
                //fclose(file_read);
                break;
              }
            }
            fclose(file_read);
            printf("get command executed successfully");

    }

    // put command execution in server side.
    else if(!(strcmp(command,"put")))
    {
          printf("Receiving file size\n");
          n = recvfrom(sockfd, buf, 1024, 0,  (struct sockaddr *)&clientaddr, &clientlen);
          if (n < 0) 
            error("ERROR in recvfrom");

          //In the case of file existing we first receive the size of the file in bytes.
          int file_size = atoi(buf);
          char *endptr;
          file_size = strtol(buf, &endptr, 10);
          bzero(buf,1024);
          printf("Receiving file %s from client  of Filesize: %d\n", file_name, file_size);
          FILE* file_write = fopen(file_name, "wb");
          if(file_write == NULL)
          {
              error("Error in opening the file to write the data that is to be recieved");
          }
          //int cumulatives_bytes_transferred=0;
          int byte_transfer_size=1024;
          for(int cumulatives_bytes_transferred=0; file_size>=cumulatives_bytes_transferred;cumulatives_bytes_transferred+=byte_transfer_size)
          {
              bzero(buf, 1024);
              // considering last transfer case.
              if(file_size- cumulatives_bytes_transferred < 1024)
              {
                byte_transfer_size= file_size- cumulatives_bytes_transferred;
              }
              n = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr *)&clientaddr, &clientlen);
              if (n < 0)
                error("ERROR in recvfrom");
              // cumulative count increments by an amount of byte_transfer_size.
              printf("Received %d bytes of data in total till now from %d in total\n",cumulatives_bytes_transferred,file_size);
              fwrite(buf, byte_transfer_size, 1, file_write);
              if(byte_transfer_size<1024)
              {
                printf("file transfer complete to client\n");
                //fclose(file_write);
                break;
              }

          }
          fclose(file_write);
    }
    else if( ! (strcmp(command,"delete")))
    {
        clientlen = sizeof(clientaddr);
        printf("Locating and getting file named %s \n", file_name);
        FILE* file_read = fopen(file_name, "rb");
        if(file_read== NULL)
        {
          // to send a message stating file does not exist.
          strcpy(buf, "donotexist");
          printf("The file %s requested  DOES NOT EXIST \n", file_name);
          n = sendto(sockfd, buf, strlen(buf), 0,(struct sockaddr *) &clientaddr, clientlen);
          if (n < 0)
            error("ERROR in sendto");
          continue;
        }
        fclose(file_read);
        if (remove(file_name) == 0) 
        {
          strcpy(buf, "deleted");
        } 
        else 
        {
          perror("Error deleting file");
          printf("deleting unsuccessfull\n");
        }
        n = sendto(sockfd, buf, strlen(buf), 0,(struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          error("ERROR in sendto");
    }
    else if ((!strcmp(command,"ls")))
    {
        DIR* directory;
        bzero(buf,1024);
        struct dirent *entry;
        directory=opendir(".");
        while((entry= readdir(directory))!= NULL)
        {
            //trcat(buf, entry);
            //strcat(buf,"\n");
            strncat(buf, entry->d_name, sizeof(entry->d_name));
            strncat(buf, "\n", 1);  // Add newline after each entry
        }
        closedir(directory);
        n = sendto(sockfd, buf, strlen(buf), 0,(struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          error("ERROR in sendto");
    }

    //exiting command
    else if (!(strcmp(command,"exit")))
    {
      break;
    }    
    /* 
     * sendto: echo the input back to the client 
     */
    else
    {
        
        n = sendto(sockfd, command, 1024, 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in sendto");
    }

  }
  return 0;
}
