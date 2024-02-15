# UDP_reliability
Network_systems

This Projects talks about communication between client and the server. Server should wait for a UDP connection after binding to requested port (ie. 5001 in the example above). User input involves an IP address and port number where the server side code is running. Depending on the commands received, the server responds to the client's request in the following manner:

get file - The server transmits the requested file to the client

put file -   The server receives the transmitted file from the client and stores it locally

delete file - The server deletes the file if it exists

ls - the server should search all the files it has in its current working directory and send a list of all these files to the client

exit - The server should acknowledge that the client is done with a goodbye message.

Files and Description:
udp_client_reliability.c : Takes user input to determine the functionality to be performed in the server and client. Coordinates the interaction between sender and receiver functions. This file implements sender and receiver functions similar to sendto and recvfrom functions with an additional functionality of Go-Back-N mechanism.

udp_server_reliability.c: Performs accordingly based on the interaction with client with the sender and receiver functions implemented.

udp_client.c : Performs same action as udp_client_reliability.c except the usage of sendto and recvfrom instead of sender and receiver functions.

udp_server.c : Performs same action as udp_server_reliability.c except the usage of sendto and recvfrom instead of sender and receiver functions.

send_receive.c : The send_receive.h file contains the implementation of a reliable data transfer mechanism using the Go-Back-N (GBN) protocol with sender and receiver functions. It utilizes UDP for connectionless communication. The sender function segments data into packets with sequence numbers, while the receiver function handles packet reception, reordering, and acknowledgment. This file offers a simple yet effective GBN implementation for reliable communication over unreliable channels. Refer to the provided comments for detailed insights into the implementation.

Submission: For this submission consider udp_client_reliability.c and udp_server_reliability.c with the usage of send_receive.c, send_receive.h included in both server and client side.
