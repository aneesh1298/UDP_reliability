#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

int sender(int sockfd, char* data, int data_size, int flags,
           const struct sockaddr *dest_addr, socklen_t addrlen) ;
int receiver(int sockfd, char* received_data, int max_data_size, int flags,
             struct sockaddr* src_addr, socklen_t* addrlen) ;

