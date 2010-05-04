#include <vector>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include "random.h"

using namespace std;

#define MAX_PACKET_SIZE       1000
#define MAX_SENT_PACKETS      100000000
#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

// Global Variables
int num_processes = -1;
int curr_process_num = -1;
int* send_sockets;
int* recv_sockets;

// Function Prototypes
void* send_data(void*);
void* recv_data(void*);
uint64_t computeCheckSum(const char* buffer, int length);
int receive(int sockfd, char* recv_buf, int length);
void fillRandomData(Random& data_rand_num, char* buf, int length);

int main(int argc, char* argv[])
{
   int base_port = -1;
   char** process_list = (char**) NULL;

   for (int i = 1; i < argc; i += 2)
   {
      if (strcmp(argv[i], "-np") == 0)
         num_processes = atoi(argv[i+1]);
      else if (strcmp(argv[i], "-id") == 0)
         curr_process_num = atoi(argv[i+1]);
      else if (strcmp(argv[i], "-bp") == 0)
         base_port = atoi(argv[i+1]);
      else
      {
         fprintf(stderr, "Unrecognized Option(%s)\n", argv[i]);
         exit(EXIT_FAILURE);
      }
   }

   fprintf(stderr, "Process(%i): Num Processes(%i), Curr Process Num(%i), Base Port(%i)\n",
         curr_process_num, num_processes, curr_process_num, base_port);
   
   if ((num_processes == -1) || (curr_process_num == -1) || (base_port == -1))
   {
      fprintf(stderr, "Uninitialized incorrectly\n");
      exit(EXIT_FAILURE);
   }

   process_list = new char*[num_processes];
   for (int i = 0; i < num_processes; i++)
   {
      process_list[i] = new char[30];
      scanf("%s", process_list[i]);
   }

   // Create a send thread
   // Create a receive thread
   // Create a pipe from the send to the receive thread

   // Create a socket for receiving connections
   int listen_socket = socket(PF_INET, SOCK_STREAM, 0);
   if (listen_socket == -1)
      handle_error("socket");

   // Bind this socket
   struct sockaddr_in listen_addr;
   memset(&listen_addr, 0, sizeof(listen_addr));

   listen_addr.sin_family = AF_INET;
   listen_addr.sin_addr.s_addr = INADDR_ANY;
   listen_addr.sin_port = htons(base_port + curr_process_num);
   if (bind(listen_socket, (struct sockaddr*) &listen_addr,
            sizeof(struct sockaddr_in)) == -1)
      handle_error("bind");

   // Do a listen
   if (listen(listen_socket, num_processes) == -1)
      handle_error("listen");

   // A sleep to allow all processes to be spawned
   sleep(5);

   // Create sockets for sending data to all other processes
   send_sockets = new int[num_processes];
   for (int i = 0; i < num_processes; i++)
   {
      send_sockets[i] = socket(PF_INET, SOCK_STREAM, 0);
      if (send_sockets[i] == -1)
         handle_error("socket");

      struct sockaddr_in serv_addr;
      memset(&serv_addr, 0, sizeof(serv_addr));
      
      serv_addr.sin_family = AF_INET;
      
      struct hostent *server;
      if ((server = gethostbyname(process_list[i])) == NULL)
      {
         fprintf(stderr, "Lookup on host(%s) failed\n", process_list[i]);
         exit(EXIT_FAILURE);
      }
      serv_addr.sin_addr.s_addr = *((unsigned long *) server->h_addr_list[0]);

      serv_addr.sin_port = htons(base_port + i);
      
      // Connect these sockets
      if (connect(send_sockets[i], (struct sockaddr*) &serv_addr,
              sizeof(struct sockaddr_in)) == -1)
         handle_error("connect");

      if (send(send_sockets[i], &curr_process_num,
              sizeof(curr_process_num), 0) == -1)
         handle_error("send");
   }

   // Accept connections
   recv_sockets = new int[num_processes]; 
   for (int i = 0; i < num_processes; i++)
   {
      struct sockaddr_in client_addr;
      memset(&client_addr, 0, sizeof(client_addr));

      socklen_t addrlen = sizeof(struct sockaddr_in);
      
      int client_socket = accept(listen_socket, (struct sockaddr*) &client_addr, &addrlen);
      if ((client_socket == -1) || (addrlen != sizeof(struct sockaddr_in)))
         handle_error("accept");

      // Get the client process num
      int client_process_num;
      if (recv(client_socket, &client_process_num,
               sizeof(client_process_num), 0) != sizeof(client_process_num))
         handle_error("recv");

      // Set the socket correctly
      recv_sockets[client_process_num] = client_socket;
   }

   // Create a thread for sending data
   pthread_t send_thread;
   if (pthread_create(&send_thread, NULL, send_data, NULL) < 0)
      handle_error("pthread_create");

   recv_data(NULL);

   if (pthread_join(send_thread, NULL) < 0)
      handle_error("pthread_join");

   // Cleanup
   delete [] send_sockets;
   delete [] recv_sockets;
   for (int i = 0; i < num_processes; i++)
      delete [] process_list[i];
   delete [] process_list;

   fprintf(stderr, "Process(%i): Exited Successfully\n", curr_process_num);
   return 0;
}

void* send_data(void*)
{
   // Variables needed
   // 1) num_processes
   // 2) send_sockets
   // 3) curr_process_num
   char send_buf[MAX_PACKET_SIZE + 100];

   Random length_rand_num;
   Random dest_rand_num;
   Random data_rand_num;

   length_rand_num.seed(curr_process_num);
   dest_rand_num.seed(curr_process_num);
   data_rand_num.seed(curr_process_num);

   int packets_sent = 0;
   while(packets_sent < MAX_SENT_PACKETS)
   {
      int length = length_rand_num.next(MAX_PACKET_SIZE) + 1;
      int dest = dest_rand_num.next(num_processes);
      uint64_t checksum;
      
      fillRandomData(data_rand_num, send_buf, length + sizeof(length) + sizeof(checksum));
      
      checksum = computeCheckSum(send_buf + sizeof(length) + sizeof(checksum), length);
      memcpy(send_buf, &length, sizeof(length));
      memcpy(send_buf + sizeof(length), &checksum, sizeof(checksum));
      
      if (send(send_sockets[dest], send_buf, length + sizeof(length) + sizeof(checksum), 0) == -1)
         handle_error("send");

      packets_sent ++;
   }
   
   // close all the connection
   for (int i = 0; i < num_processes; i++)
   {
      if (close(send_sockets[i]) == -1)
         handle_error("close");
   } 
}

void* recv_data(void*)
{
   // Variables
   // 1) num_processes
   // 2) recv_sockets 
   char recv_buf[MAX_PACKET_SIZE];
   vector<bool> connection_terminated(num_processes, false);

   fd_set readset;
   int packet_num = 0;   

   int num_connections_terminated = 0;
   while (num_connections_terminated < num_processes)
   {
      FD_ZERO(&readset);
      int maxfd = 0;
      for (int i = 0; i < num_processes; i++)
      {
         if (!connection_terminated[i])
         {
            FD_SET(recv_sockets[i], &readset);
            maxfd = (recv_sockets[i] > maxfd) ? recv_sockets[i] : maxfd;
         }
      }

      int ready_fds = select(maxfd + 1, &readset, NULL, NULL, NULL);
      
      if (ready_fds < 1)
      {
         fprintf(stderr, "ready_fds(%i)", ready_fds);
         exit(EXIT_FAILURE);
      }

      for (int i = 0; i < num_processes; i++)
      {
         if (FD_ISSET(recv_sockets[i], &readset))
         {
            int length;
            uint64_t checksum;
            int num_bytes;
            
            num_bytes = receive(recv_sockets[i], (char*) &length, sizeof(length));
            if (num_bytes == 0)
            {
               // The socket is closed
               if (close(recv_sockets[i]) == -1)
                  handle_error("close");

               connection_terminated[i] = true;
               // Continue to check the recv_socket associated with the next process
               continue;
            }
            if (num_bytes != sizeof(length))
            {
               fprintf(stderr, "(1) recv: num_received(%i), num_expected(%i), packet_num(%i)\n",
                     num_bytes, sizeof(length), packet_num);
               exit(EXIT_FAILURE);
            }
            
            num_bytes = receive(recv_sockets[i], (char*) &checksum, sizeof(checksum));
            if (num_bytes != sizeof(checksum))
            {
               fprintf(stderr, "(2) recv: num_received(%i), num_expected(%i), packet_num(%i)\n",
                     num_bytes, sizeof(checksum), packet_num);
               exit(EXIT_FAILURE);
            }

            num_bytes = receive(recv_sockets[i], recv_buf, length);
            if (num_bytes != length)
            {
               fprintf(stderr, "(3) recv: num_received(%i), num_expected(%i), packet_num(%i)\n",
                     num_bytes, length, packet_num);
               exit(EXIT_FAILURE);
            }

            if (checksum != computeCheckSum(recv_buf, length))
            {
               fprintf(stderr, "CheckSum computation failed: got(%llu), expected(%llu)\n",
                     checksum, computeCheckSum(recv_buf, length));
               exit(EXIT_FAILURE);
            }

            packet_num ++;
         }
      }

      num_connections_terminated = 0;
      for (int i = 0; i < num_processes; i++)
         num_connections_terminated += (int) connection_terminated[i];
   }
}

int receive(int sockfd, char* recv_buf, int length)
{
   int num_bytes = 0;
   while (num_bytes < length)
   {
      int result = recv(sockfd, recv_buf + num_bytes, length - num_bytes, 0);
      if (result < 0)
      {
         fprintf(stderr, "recv: num_received(%i), num_expected(%i)\n",
               result, length - num_bytes);
         exit(EXIT_FAILURE);
      }
      else if (result == 0)
      {
         if (num_bytes != 0)
         {
            fprintf(stderr, "num_bytes(%i)\n", num_bytes);
            exit(EXIT_FAILURE);
         }
         break;
      }
      num_bytes += result;
   }

   return num_bytes;
}

void fillRandomData(Random& data_rand_num, char* buf, int length)
{
   for (int i = 0; i < length; i++)
   {
      buf[i] = data_rand_num.next(256);
   }
}

uint64_t computeCheckSum(const char* buffer, int length)
{
   uint64_t checksum = 0;
   for (int i = 0; i < length; i++)
   {
      checksum += (uint64_t) buffer[i];
   }
   return checksum;
}
