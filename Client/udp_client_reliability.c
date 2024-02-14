
/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
#include "send_receive.h"

#define BUFSIZE 1024
#define NMBEB 1

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char command[BUFSIZE];
    char file_name[BUFSIZE];

    /* check command line arguments are valid in count */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       //exit(0);
    }
    hostname = argv[1];
    // converting string input into an integer.
    portno = atoi(argv[2]);
    if(portno < 5001 || portno > 65534)
    {
        error("Port should be grater than 5000 and less than 65535");
    }

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    while(1)
    {
      /* get a message from the user */
      bzero(buf, BUFSIZE);
      bzero(command, BUFSIZE);
      bzero(file_name, BUFSIZE);
      printf("Please enter msg: ");
      fgets(buf, BUFSIZE, stdin);

      //use string tokenization to capture different parts of the command
      char *token = strtok(buf, " \t\n");
      if (token != NULL) 
      {
          strcpy(command, token); // Copy the first token to cmd
          token = strtok(NULL, " \t\n"); // Get the next token
          if (token != NULL) 
          {
              strcpy(file_name, token); // Copy the second token to fname
          }
      }
        /* send the message to the server  showing client details*/
        serverlen = sizeof(serveraddr);
        n = sender(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sender");
        n = sender(sockfd, command, strlen(command), 0, (struct sockaddr*)&serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sender");
        n = sender(sockfd, file_name, strlen(file_name), 0, (struct sockaddr*)&serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sender");

      bzero(buf,1024);
      if(!(strcmp(command,"get")))
      {    
          /* print the server's reply */
          n = receiver(sockfd, buf, 1024, 0,(struct sockaddr*) &serveraddr, &serverlen);
          if (n < 0) 
            error("ERROR in receiver");

            // Receiver message checking for whether the file exists or not.
            if(!strcmp(buf, "donotexist"))
            { //If server responds that the file doesn't exist
              printf("The file %s requested from server DOES NOT EXIST \n", file_name);
              // continues to next iteration of while loop
              continue;
            }

          //In the case of file existing we first receive the size of the file in bytes.
            int file_size = atoi(buf);
            bzero(buf,1024);
            printf("Receiving file %s from client of Filesize: %d\n", file_name, file_size);
            FILE* file_write = fopen(file_name, "wb");
            if(file_write == NULL)
            {
                error("Error in opening the file to write the data that is to be recieved");
            }
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
              n = receiver(sockfd, buf, byte_transfer_size, 0, (struct sockaddr *)&serveraddr, &serverlen);
              if (n < 0)
                error("ERROR in receiver");
              // cumulative count increments by an amount of byte_transfer_size.
              printf("Received %d bytes of data in total till now from %d in total\n",cumulatives_bytes_transferred,file_size);
              fwrite(buf, byte_transfer_size, NMBEB, file_write);
              if(byte_transfer_size<1024)
              {
                printf("file transfer complete to client\n");
                fclose(file_write);
                break;
              }
            }
            // get operation completed successfully.
            //fclose(file_write);
            printf("get command executed successfully\n");
      }

      // command put is executed now.
      else if (!(strcmp(command,"put")))
      {
        printf("File to be transferred is : %s\n",file_name);
        FILE* file_read= fopen(file_name,"rb");
        if(file_read== NULL)
        {
          printf("The file %s requested  DOES NOT EXIST \n", file_name);
          continue;
        }
        // serverlen = sizeof(serveraddr);
        // n = sender(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
        // if (n < 0) 
        //   error("ERROR in sender");

        // now should send the file size to the server.
        // Seek to the end of the file to determine its size
        fseek(file_read, 0, SEEK_END);
        // Get the file size
        int file_size= ftell(file_read);
        //fclose(file_read);
        printf("File_size is determined as %d \n",file_size);
        bzero(buf, 1024);
        sprintf(buf, "%d", file_size);
        n = sender(sockfd, buf, strlen(buf), 0,
               (struct sockaddr *) &serveraddr, serverlen);
        if (n < 0)
          error("ERROR in sender");
        bzero(buf, 1024);
        fseek(file_read, 0, SEEK_SET);
        // have to send file data 1024 bytes at atime.
          //int cumulatives_bytes_transferred=0;
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
            n = sender(sockfd, buf, bytes_sent, 0, (struct sockaddr *)&serveraddr, serverlen);
            if (n < 0)
              error("ERROR in sender");
            // cumulative count increcvfrmements by an amount of byte_transfer_size.
            printf("Sent %d bytes of data in total till now from %d in total\n",cumulatives_bytes_transferred,file_size);
              
            if(byte_transfer_size<1024)
            {
                printf("file transfer complete from  server\n");
                fclose(file_read);
                break;
            }
          }
          //fclose(file_read);
          // get operation completed successfully.
          printf("put command executed successfully\n");
      }

      // to implement delete command
      else if (!strcmp(command,"delete"))
      {
        // receiving whether file does not exist or 
        n = receiver(sockfd, buf, 1024, 0,(struct sockaddr*) &serveraddr, &serverlen);
        if (n < 0) 
          error("ERROR in receiver");  
        // Receiver message checking for whether the file exists or not.
        if(!strcmp(buf, "donotexist"))
        { //If server responds that the file doesn't exist
            printf("The file %s requested from server DOES NOT EXIST \n", file_name);
            // continues to next iteration of while loop
            continue;
        } 
        else if (!strcmp(buf, "deleted"))
        {
            printf("%s file is deleted successfully\n",file_name);
        }
        else
        {
            printf("%s file is not deleted \n",file_name);
        }

      }
      // ls command execution
      else if(!(strcmp(command,"ls")))
      {
          n = receiver(sockfd, buf, 1024, 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0)
            error("ERROR in receiver");
          printf("Sucessfully listing the files below:\n");
          printf("%s\n",buf);

      }
      else if(!(strcmp(command,"exit")))
      {
          close(sockfd);
          printf("Exiting the program with removing server connection\n");
          break;
      }
      else
      {
        //echoing back the invalid input
            n = receiver(sockfd, buf, 1024, 0, (struct sockaddr *)&serveraddr, &serverlen);
            if (n < 0)
                error("ERROR in receiver");
            printf("Invalid command\t");
            printf("%s\n", buf);
      }
    }

     return 0;
}
