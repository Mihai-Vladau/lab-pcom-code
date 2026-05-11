#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "common.h"
#include "queue.h"
#include "utils.h"

#define TICK(X)                                                                \
  struct timespec X;                                                           \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X)

#define TOCK(X)                                                                \
  struct timespec X##_end;                                                     \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X##_end);                                \
  printf("Total time = %f seconds\n",                                          \
         (X##_end.tv_nsec - (X).tv_nsec) / 1000000000.0 +                      \
             (X##_end.tv_sec - (X).tv_sec))

/* Max size of the datagrams that we will be sending */
#define CHUNKSIZE 1024
#define SENT_FILENAME "file.bin"
#define SERVER_IP "172.16.0.100"

/* Queue we will use for datagrams */
queue datagram_queue;

void send_file_start_stop(int sockfd, struct sockaddr_in server_address,
                          char *filename) {

  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");
  int rc;

  while (1) {
    /* Reads a chunk of the file */
    struct seq_udp d;
    int n = read(fd, d.payload, sizeof(d.payload));
    DIE(n < 0, "read");
    d.len = n;

    /* TODO 1.2: Send the datagram. */
    rc = sendto(sockfd, &d, sizeof(struct seq_udp), 0,
                  (struct sockaddr *)&server_address, sizeof(server_address));
    int ack;
    recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
    /* TODO 1.3: Wait for ACK before moving to the next datagram to send. */
    
    if (n == 0) // end of file
      break;

  }
}

void send_file_window(int sockfd, struct sockaddr_in server_address,
                      char *filename) {

  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");
  int rc;

  /* TODO 2.1: Increase window size to a value that optimally uses the link */
  int window_size = 24;
  int len = 0;
  while (1) {
    /* TODO: 1.1 Read all the data of the and add it as datagrams in
     * datagram_queue */
    /* Reads the content of a file */
    struct seq_udp *d = malloc(sizeof(struct seq_udp));
    int n = read(fd, d->payload, sizeof(d->payload));
    DIE(n < 0, "read");
    d->len = n;
    len++;
    queue_enq(datagram_queue, d);

    if (n == 0) // end of file
      break;
  }

   struct seq_udp *t = queue_deq(datagram_queue);

  /* TODO 2.2: Send window_size packets from the queue. Don't forget to free the
   * data. */
   window_size = (window_size > len) ?  len : window_size;
   int flight_packet = 0;
  while (window_size > flight_packet ) {    
    rc = sendto(sockfd, t, sizeof(struct seq_udp), 0,
                  (struct sockaddr *)&server_address, sizeof(server_address));
    flight_packet ++;
    free(t);
    t = queue_deq(datagram_queue);
  }
  // presupunem ca nu se face retransmisie 
  while (flight_packet != 0) {
      int ack;
      recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
      flight_packet--;
      if (!queue_empty(datagram_queue) && flight_packet < window_size) {
        t = queue_deq(datagram_queue);
        flight_packet++;
          rc = sendto(sockfd, t, sizeof(struct seq_udp), 0,
                  (struct sockaddr *)&server_address, sizeof(server_address));
        free(t);
      }
  }



  /* TODO 2.2: On ACK, slide the window by popping the queue and sending the
   * next datagram. */
}

void send_a_message(int sockfd, struct sockaddr_in server_address) {
  struct seq_udp d;
  strcpy(d.payload, "Hello world!");
  d.len = strlen("Hello world!");

  /* Send a UDP datagram. Sendto is implemented in the kernel (network stack of
   * it), it basically creates a UDP datagram, sets the payload to the data we
   * specified in the buffer, and the completes the IP header and UDP header
   * using the sever_address info.*/
  int rc = sendto(sockfd, &d, sizeof(struct seq_udp), 0,
                  (struct sockaddr *)&server_address, sizeof(server_address));

  DIE(rc < 0, "send");

  /* Receive the ACK. recvfrom is blocking with the current parameters */
  int ack;
  rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
}

int main(int argc, char *argv[]) {

  /* We use this structure to store the server info. IP address and Port.
   * This will be written by the UDP implementation into the header */
  struct sockaddr_in servaddr;
  int sockfd, rc;

  // for benchmarking
  TICK(TIME_A);
  
  /* Queue that we will use when implementing sliding window */
  datagram_queue = queue_create();

  // Creating socket file descriptor. SOCK_DGRAM for UDP
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(sockfd < 0, "socket");

  // Fill the information that will be put into the IP and UDP header to
  // identify the target process (via PORT) on a given host (via SEVER_IP)
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  inet_aton(SERVER_IP, &servaddr.sin_addr);

  /* TODO: Read the demo function.
  Implement and test (one at a time) each of the proposed versions for sending a
  file. */
  //send_a_message(sockfd, servaddr);
  // send_file_start_stop(sockfd, servaddr, SENT_FILENAME);
  send_file_window(sockfd, servaddr, SENT_FILENAME);

  /* Print the runtime of the program */
  TOCK(TIME_A);
  
  close(sockfd);

  return 0;
}
