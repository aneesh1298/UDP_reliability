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
#include "send_receive.h"

#define MAX_PACKET_SIZE 16
#define WINDOW_SIZE 4
#define TIMEOUT_MS 600

int sender(int sockfd, char* data, int data_size, int flags,
           const struct sockaddr *dest_addr, socklen_t addrlen) 
           
    {
        char buf[MAX_PACKET_SIZE+4];
        //int time[4];
        int total_bytes_sent = 0;
        sendto(sockfd, &data_size, sizeof(int), flags, (struct sockaddr*)dest_addr, addrlen);
        int last_ack_received = 0;
        int last_frame_sent = 0;
        int num_frames = (data_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
        struct timeval start_time, current_time;
        //gettimeofday(&start_time, NULL);
        int set_count= (num_frames+4)/4;
        int current_set=0;
        if(num_frames%4 ==0)
        {
            set_count=set_count-1;
        }
        int last_packet_size;
        if(data_size % MAX_PACKET_SIZE ==0)
        {
            last_packet_size=MAX_PACKET_SIZE;
        }
        else{
            last_packet_size=data_size % MAX_PACKET_SIZE;
        }
        while ((last_ack_received < num_frames)) 
        {
            int i=1;
            last_frame_sent=last_ack_received;
            while(last_frame_sent - last_ack_received < WINDOW_SIZE)
            {
                int frame_size = (last_frame_sent == num_frames - 1) ? last_packet_size : MAX_PACKET_SIZE;
                last_frame_sent++;
                memcpy(buf, &last_frame_sent, sizeof(int));
                memcpy(buf+4, data + (last_ack_received+i-1) * MAX_PACKET_SIZE, frame_size);
                int bytes_sent = sendto(sockfd, buf, frame_size+4, flags, (struct sockaddr*)dest_addr, addrlen);
                if (bytes_sent < 0) {
                    perror("ERROR in sendto");
                    exit(1);
                }
                total_bytes_sent += bytes_sent;
                i++;
                usleep(10000);
            }
            gettimeofday(&start_time, NULL);
            while(1)
            {
                gettimeofday(&current_time, NULL);
                long elapsed_time = (current_time.tv_sec - start_time.tv_sec) * 1000 + (current_time.tv_usec - start_time.tv_usec) / 1000;
                if(elapsed_time>=700)
                {
                    break;
                }
            }
            int ack_number;
            int bytes_received = recvfrom(sockfd, &ack_number, sizeof(int), flags, (struct sockaddr*)dest_addr, &addrlen);
            if (bytes_received > 0 && ack_number!=-1) 
            {
                last_ack_received = ack_number;
                //current_set++;

            }
            //current_set++;

        }
    }


int receiver(int sockfd, char* received_data, int max_data_size, int flags,
             struct sockaddr* src_addr, socklen_t* addrlen) 
    {
        char buf[MAX_PACKET_SIZE+4];
        int frame_number;
        //char data_size[4];
        int frame_sequence_number[4]={100,100,100,100};
        int n=recvfrom(sockfd, &max_data_size, sizeof(int), flags, (struct sockaddr*)src_addr, addrlen);
        //max_data_size=atoi(data_size);
        int expected_frame = 0;
        int num_frames = (max_data_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
        int total_bytes_received=0;
        int sequence_number_to_send=0;
        int set_count= (num_frames+4)/4;
        int current_set=0;
        // if(num_frames%4 ==0)
        // {
        //     set_count=set_count-1;
        // }
        int last_packet_size;
        if(max_data_size % MAX_PACKET_SIZE ==0)
        {
            last_packet_size=MAX_PACKET_SIZE;
        }
        else{
            last_packet_size=max_data_size % MAX_PACKET_SIZE;
        }
        while (expected_frame < num_frames) 
        {
            for(int i=0;i<4;i++)
            {
                int frame_size = (expected_frame == num_frames - 1) ? last_packet_size : MAX_PACKET_SIZE;
                expected_frame++;
                int bytes_received = recvfrom(sockfd, buf, frame_size+4, flags, (struct sockaddr*)src_addr, addrlen);
                if (bytes_received < 0) 
                {
                    frame_sequence_number[i]=100;
                    //exit(1);
                    break;

                }
                memcpy(&frame_number,buf,sizeof(int));
                memcpy(received_data+total_bytes_received,buf+4,frame_size);
                total_bytes_received=total_bytes_received+frame_size;
                frame_sequence_number[i]=frame_number;
            }
            int sequence_number_to_send = -1;  // Default value if 100 is not found
            for (int i = 0; i < 4; i++) 
            {
                if (frame_sequence_number[i] == 100) 
                {
                        if(i==0) 
                        {
                            expected_frame=expected_frame-4;
                            break;
                        }
                        sequence_number_to_send= frame_sequence_number[i-1];
                        expected_frame= expected_frame-4+i;
                        break;

                    //break;  // Exit the loop once 100 is found
                }
                else{
                        sequence_number_to_send = frame_sequence_number[i];
                }
            }
            int bytes_sent = sendto(sockfd, &sequence_number_to_send, 4, flags, (struct sockaddr*)src_addr, *addrlen);
            if (bytes_sent>0 && sequence_number_to_send!=-1)
            {
                //
                //current_set++;
                //expected_frame= expected_frame-sequence_number_to_send;
            }
            
            
        }
    }
