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
#include <sys/time.h>
#include <time.h>

#include "common.h"
#include "utils.h"
#include "list.h"

/* Max size of the datagrams that we will be sending */
#define CHUNKSIZE MAX_SIZE
#define SENT_FILENAME "file.bin"
#define SERVER_IP "172.16.0.100"

#define TICK(X)                                                                \
  struct timespec X;                                                           \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X)

#define TOCK(X)                                                                \
  struct timespec X##_end;                                                     \
  clock_gettime(CLOCK_MONOTONIC_RAW, &X##_end);                                \
  printf("Total time = %f seconds\n",                                          \
         (X##_end.tv_nsec - (X).tv_nsec) / 1000000000.0 +                      \
             (X##_end.tv_sec - (X).tv_sec))

list *window;

void send_file_start_stop(int sockfd, struct sockaddr_in server_address,
                          char *filename) {

  int fd = open(filename, O_RDONLY);
  DIE(fd < 0, "open");
  int rc;
  int seq = 0;

  while (1) {
    struct seq_udp d;
    int n = read(fd, d.payload, sizeof(d.payload));
    DIE(n < 0, "read");
    
    // Tratarea pachetului de final (EOF)
    if (n == 0) {
        d.len = 0;
        d.seq = seq;
        int retries = 0;
        
        rc = sendto(sockfd, &d, sizeof(struct seq_udp), 0,
                    (struct sockaddr *)&server_address, sizeof(server_address));
        int ack;
        rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
        
        // Incercam de maxim 5 ori sa inchidem frumos
        while ((rc < 0 || ack != d.seq) && retries < 5) {
            rc = sendto(sockfd, &d, sizeof(struct seq_udp), 0,
                        (struct sockaddr *)&server_address, sizeof(server_address));
            rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
            retries++;
        }
        break; // Iesim din bucla mare, am terminat fisierul
    }

    d.len = n;
    d.seq = seq;

    // Bucla de Stop-and-Wait pentru pachetul curent
    while (1) {
        rc = sendto(sockfd, &d, sizeof(struct seq_udp), 0,
                    (struct sockaddr *)&server_address, sizeof(server_address));
        
        int ack;
        rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);

        if (rc < 0) {
            // TIMEOUT: nu a raspuns, bucla "while(1)" se reia si face sendto iar
            continue;
        } else if (ack == d.seq) {
            // SUCCESS: am primit confirmarea corecta!
            seq++; // Trecem la urmatorul numar de secventa
            break; // Iesim din bucla de retransmisie
        }
        // Daca am primit un ack gresit, ignoram si lasam sa dea timeout
    }
  }
}

void send_file_go_back_n(int sockfd, struct sockaddr_in server_address,
                      char *filename) {
    int fd = open(filename, O_RDONLY);
    DIE(fd < 0, "open");
    int rc;

    int window_size = 5;
    window->max_seq = 5;
    int seq = 0;
    int crt_list = 0;

    while (1) {
        struct seq_udp *d = malloc(sizeof(struct seq_udp));
        int n = read(fd, d->payload, sizeof(d->payload));
        DIE(n < 0, "read");
        if (n == 0) {
            free(d);
            break;
        }
        d->len = n;
        d->seq = seq;
        add_list_elem(window, d, sizeof(struct seq_udp), seq);
        seq++;
        crt_list++;
    }

    window_size = (window_size < crt_list) ? window_size : crt_list;

    int flight_packets = 0;
    struct seq_udp t;
    struct cel *crt = window->head;

    while (flight_packets < window_size && crt != NULL) {
        t = *(struct seq_udp *)crt->info;
        rc = sendto(sockfd, &t, sizeof(struct seq_udp), 0,
                    (struct sockaddr *)&server_address, sizeof(server_address));
        DIE(rc < 0, "sendto");
        flight_packets++;
        crt = crt->next;
    }

    int expected_seq = 0;
    while (flight_packets != 0) {
        int ack = 0;
        rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);

        if (rc < 0) {
            struct cel* resend = window->head;
            for (int i = 0; i < flight_packets && resend != NULL; i++) {
                struct seq_udp p = *(struct seq_udp *)resend->info;
                sendto(sockfd, &p, sizeof(struct seq_udp), 0,
                       (struct sockaddr *)&server_address, sizeof(server_address));
                resend = resend->next;
            }
        } 
        else if (ack == expected_seq) {
            flight_packets--;
            struct cel *copy = window->head->next;
            free(window->head);
            window->head = copy;
            expected_seq++;

            if (crt != NULL) {
                struct seq_udp p = *(struct seq_udp *)crt->info;
                rc = sendto(sockfd, &p, sizeof(struct seq_udp), 0,
                            (struct sockaddr *)&server_address, sizeof(server_address));
                flight_packets++;
                crt = crt->next;
            }
        }
    }

    // Pachetul EOF (End Of File)
    struct seq_udp eof_pkt;
    eof_pkt.len = 0;
    eof_pkt.seq = seq;
    
    int eof_retries = 0; // Contor pentru reîncercări
    
    rc = sendto(sockfd, &eof_pkt, sizeof(struct seq_udp), 0,
                (struct sockaddr *)&server_address, sizeof(server_address));
    int ack;
    rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
    
    // Aici stătea blocat: acum încearcă de maxim 5 ori!
    while ((rc < 0 || ack != eof_pkt.seq) && eof_retries < 5) {
        rc = sendto(sockfd, &eof_pkt, sizeof(struct seq_udp), 0,
                    (struct sockaddr *)&server_address, sizeof(server_address));
        rc = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
        eof_retries++;
    }
}

int main(int argc, char *argv[]) {
  struct sockaddr_in servaddr;
  int sockfd, rc;

  TICK(TIME_A);

  window = create_list();

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(sockfd < 0, "socket");

  struct timeval timeout;      
  timeout.tv_sec = 0;        
  timeout.tv_usec = 50000;   // 50 milisecunde
    
  rc = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  DIE(rc < 0, "setsockopt");

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  inet_aton(SERVER_IP, &servaddr.sin_addr);

  send_file_go_back_n(sockfd, servaddr, SENT_FILENAME);

  close(sockfd);
  TOCK(TIME_A);

  return 0;
}