#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len) {

  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  int rc;

      while(bytes_remaining > 0) {
         rc = recv(sockfd, buffer, len, 0);
         bytes_received = bytes_received + rc;
         bytes_remaining = bytes_remaining - rc;
      }

  
  //return recv(sockfd, buffer, len, 0);
  return bytes_received;
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  int rc;
      while(bytes_remaining > 0) {
        rc = send(sockfd, buffer, len, 0);
        bytes_sent = bytes_sent + rc;
        bytes_remaining = bytes_remaining - rc;
    
      }
    return bytes_sent;

}