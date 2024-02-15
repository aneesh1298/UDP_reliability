#include "send_receive.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PACKET_SIZE 16
#define WINDOW_SIZE 4
#define TIMEOUT_MS 600
/**
 * @brief Send data over a socket.
 *
 * This function sends data over the specified socket to the specified
 * destination address. It is a wrapper around the sendto() system call and
 * provides an interface for sending data using UDP or other connectionless
 * socket types.
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
int sender(int sockfd, char *data, int data_size, int flags,
           const struct sockaddr *dest_addr, socklen_t addrlen)

{
  // initial 4 bytes contain sequence number.
  // REmaining 16 bytes for data transfer as apacket.
  char buf[MAX_PACKET_SIZE + 4];
  // to store the total bytes sent till now.
  int total_bytes_sent = 0;
  // sends the file_size it is going to send to the receiver function.
  sendto(sockfd, &data_size, sizeof(int), flags, (struct sockaddr *)dest_addr,
         addrlen);
  // Using a SWS of 4 and using LAR,LFS to have a check on frames.
  int last_ack_received = 0;
  int last_frame_sent = 0;
  // determines the total number of packets to be sent.
  int num_frames = (data_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
  struct timeval start_time, current_time;
  int set_count = (num_frames + 4) / 4;
  int current_set = 0;
  if (num_frames % 4 == 0) {
    set_count = set_count - 1;
  }
  // to determine the last packet size to be sent in case if bytes to send
  // doesnt divide with 16 exactly.
  int last_packet_size;
  if (data_size % MAX_PACKET_SIZE == 0) {
    last_packet_size = MAX_PACKET_SIZE;
  } else {
    last_packet_size = data_size % MAX_PACKET_SIZE;
  }
  // iterates till every frame has been sent.
  while ((last_ack_received < num_frames)) {
    int i = 1;
    // before the packet sent both are matched
    last_frame_sent = last_ack_received;
    // sends 4 packets in a sequence.
    while (last_frame_sent - last_ack_received < WINDOW_SIZE) {
      // frame size determines the size of packet. In general last packet to be
      // sent varies in size.
      int frame_size = (last_frame_sent == num_frames - 1) ? last_packet_size
                                                           : MAX_PACKET_SIZE;
      last_frame_sent++;
      // copies data of the sequence number of packet into first 4 bytes.
      memcpy(buf, &last_frame_sent, sizeof(int));
      // copies actual data into next 16 bytes.
      memcpy(buf + 4, data + (last_ack_received + i - 1) * MAX_PACKET_SIZE,
             frame_size);
      // sends actual frame_size + 4bytes of seq number.
      int bytes_sent = sendto(sockfd, buf, frame_size + 4, flags,
                              (struct sockaddr *)dest_addr, addrlen);
      if (bytes_sent < 0) {
        perror("ERROR in sendto");
        exit(1);
      }
      // total bytes sent has been updated.
      total_bytes_sent += bytes_sent;
      i++;
      usleep(10000);
    }
    gettimeofday(&start_time, NULL);
    // wait for 700msec. THis is double the max RTT that my machine has
    // provided.
    while (1) {
      gettimeofday(&current_time, NULL);
      long elapsed_time = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                          (current_time.tv_usec - start_time.tv_usec) / 1000;
      if (elapsed_time >= 700) {
        break;
      }
    }
    // receives ack_number from the receiver side.
    int ack_number;
    int bytes_received = recvfrom(sockfd, &ack_number, sizeof(int), flags,
                                  (struct sockaddr *)dest_addr, &addrlen);
    // actually data has been sent and receiver sends the ack number. Then
    // last_ack_received is accordingly set for next iteration.
    if (bytes_received > 0 && ack_number != -1) {
      last_ack_received = ack_number;
      // current_set++;
    }
    // current_set++;
  }
}

/**
 * @brief Receive data on a socket.
 *
 * This function receives data on the specified socket and stores it in the
 * provided buffer. It is a wrapper around the recvfrom() system call and is
 * designed for use with connectionless socket types, such as UDP.
 *
 * @param sockfd         The socket file descriptor to use for receiving data.
 * @param received_data  A pointer to the buffer where the received data will be
 * stored.
 * @param max_data_size  The maximum size of the buffer to prevent buffer
 * overflow.
 * @param flags          Optional flags to modify the behavior of the receive
 * operation. See the recvfrom() documentation for flag options.
 * @param src_addr       A pointer to the source address structure (struct
 * sockaddr) where the sender's address information will be stored.
 * @param addrlen        A pointer to the size of the source address structure.
 *                       It should be initialized to the size of the structure
 * before the call. Upon return, it will be updated with the actual size of the
 * source address.
 *
 * @return On success, the number of bytes received is returned. On failure,
 *         -1 is returned, and errno is set to indicate the error.*/
int receiver(int sockfd, char *received_data, int max_data_size, int flags,
             struct sockaddr *src_addr, socklen_t *addrlen) {
  // initial 4 bytes contain sequence number.
  // REmaining 16 bytes for data transfer as apacket.
  char buf[MAX_PACKET_SIZE + 4];
  int frame_number;
  // THis contains the seq number of the packets received.
  int frame_sequence_number[4] = {100, 100, 100, 100};
  // receives the size of the message arriving and stores it in specific
  // location.
  int n = recvfrom(sockfd, &max_data_size, sizeof(int), flags,
                   (struct sockaddr *)src_addr, addrlen);
  // Sets the expected_frame to 0.
  int expected_frame = 0;
  // determines number of packets to send actually.
  int num_frames = (max_data_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
  int total_bytes_received = 0;
  // determines the return value of the function to be send.
  int sequence_number_to_send = 0;
  int set_count = (num_frames + 4) / 4;
  int current_set = 0;
  // if(num_frames%4 ==0)
  // {
  //     set_count=set_count-1;
  // }
  // to determine the last packet size to be received in case if bytes to send
  // doesnt divide with 16 exactly.
  int last_packet_size;
  if (max_data_size % MAX_PACKET_SIZE == 0) {
    last_packet_size = MAX_PACKET_SIZE;
  } else {
    last_packet_size = max_data_size % MAX_PACKET_SIZE;
  }
  // iterates till last frame is received.
  while (expected_frame < num_frames) {
    // iterates for 4 packets.
    for (int i = 0; i < 4; i++) {
      int frame_size = (expected_frame == num_frames - 1) ? last_packet_size
                                                          : MAX_PACKET_SIZE;
      expected_frame++;
      int bytes_received = recvfrom(sockfd, buf, frame_size + 4, flags,
                                    (struct sockaddr *)src_addr, addrlen);
      // if any packet did not get valid bytes then we consider packet is not
      // received.
      if (bytes_received < 0) {
        frame_sequence_number[i] = 100;
        // exit(1);
        break;
      }
      // takes the frame sequence number and content and stores them exactly.
      memcpy(&frame_number, buf, sizeof(int));
      memcpy(received_data + total_bytes_received, buf + 4, frame_size);
      // this acts as an offset where data isto be stored.
      total_bytes_received = total_bytes_received + frame_size;
      frame_sequence_number[i] = frame_number;
    }
    // initially this is initialised to -01.
    int sequence_number_to_send = -1; // Default value if 100 is not found
    for (int i = 0; i < 4; i++) {
      // if 1st packet itself not received then expected frame went back to
      // normal.
      if (frame_sequence_number[i] == 100) {
        if (i == 0) {
          expected_frame = expected_frame - 4;
          break;
        }
        // tells the prev received valid one to be sent as ack_receivbed.
        sequence_number_to_send = frame_sequence_number[i - 1];
        // expected frame is modified accordingly.
        expected_frame = expected_frame - 4 + i;
        break;

        // break;  // Exit the loop once 100 is found
      } else {
        sequence_number_to_send = frame_sequence_number[i];
      }
    }
    // sends the ack_number that it has been effectively received.
    int bytes_sent = sendto(sockfd, &sequence_number_to_send, 4, flags,
                            (struct sockaddr *)src_addr, *addrlen);
    if (bytes_sent > 0 && sequence_number_to_send != -1) {
    }
  }
}
