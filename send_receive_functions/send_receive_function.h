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
/**
 * @brief Send data over a socket.
 *
 * This function sends data over the specified socket to the specified destination
 * address. It is a wrapper around the sendto() system call and provides an
 * interface for sending data using UDP or other connectionless socket types.
 *
 * @param sockfd    The socket file descriptor to use for sending data.
 * @param data      A pointer to the buffer containing the data to be sent.
 * @param data_size The size of the data to be sent in bytes.
 * @param flags     Optional flags to modify the behavior of the send operation.
 *                  See the sendto() documentation for flag options.
 * @param dest_addr A pointer to the destination address structure
 *                  (struct sockaddr) representing the target address where
 *                  the data will be sent.
 * @param addrlen   The size of the destination address structure.
 *
 * @return On success, the number of bytes sent is returned. On failure,
 *         -1 is returned, and errno is set to indicate the error.*/
int sender(int sockfd, char* data, int data_size, int flags,
           const struct sockaddr *dest_addr, socklen_t addrlen) ;
/**
 * @brief Receive data on a socket.
 *
 * This function receives data on the specified socket and stores it in the
 * provided buffer. It is a wrapper around the recvfrom() system call and is
 * designed for use with connectionless socket types, such as UDP.
 *
 * @param sockfd         The socket file descriptor to use for receiving data.
 * @param received_data  A pointer to the buffer where the received data will be stored.
 * @param max_data_size  The maximum size of the buffer to prevent buffer overflow.
 * @param flags          Optional flags to modify the behavior of the receive operation.
 *                       See the recvfrom() documentation for flag options.
 * @param src_addr       A pointer to the source address structure (struct sockaddr)
 *                       where the sender's address information will be stored.
 * @param addrlen        A pointer to the size of the source address structure.
 *                       It should be initialized to the size of the structure before the call.
 *                       Upon return, it will be updated with the actual size of the source address.
 *
 * @return On success, the number of bytes received is returned. On failure,
 *         -1 is returned, and errno is set to indicate the error.*/
int receiver(int sockfd, char* received_data, int max_data_size, int flags,
             struct sockaddr* src_addr, socklen_t* addrlen) ;

