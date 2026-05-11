#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define MSG_MAXSIZE 1460
#define time_to_wait 60

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  int rc;
      while(bytes_remaining > 0) {
        rc = send(sockfd, buffer, bytes_remaining, 0);
        bytes_sent = bytes_sent + rc;
        bytes_remaining = bytes_remaining - rc;
        buffer = buffer + rc;
      }
    return bytes_sent;

}


int main() {
    int sockfd;
    int rc;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5001);
    rc = inet_pton(AF_INET, "172.16.0.100", &serv_addr.sin_addr.s_addr);
    
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("Conectat la serverul 172.16.0.100:5001\n");
    char sent_packet[MSG_MAXSIZE + 1];
    int i;
   memset(sent_packet, 230, MSG_MAXSIZE);
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    double elapsed_time = 0;
    int bytes_sent = 0;
    printf("Incep trimiterea pachetelor timp de 10 secunde\n");
    while (1) {
        bytes_sent = bytes_sent + send_all(sockfd, &sent_packet, sizeof(sent_packet));
        clock_gettime(CLOCK_MONOTONIC, &current); 
        elapsed_time = (current.tv_sec - start.tv_sec) + 
                              (current.tv_nsec - start.tv_nsec) / 1e9;
        if(elapsed_time > time_to_wait) {
            break;
        }                      
    }
    double throughput_mbps = (bytes_sent * 8.0) / (elapsed_time * 1000000.0);
    printf("Troughputul este de %f Mbps\n", throughput_mbps);
    return 0;
}