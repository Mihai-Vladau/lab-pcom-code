#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;

        
    // Ex 1.1: GET dummy from main server
    sockfd = open_connection("3.248.203.233", 8080, AF_INET, SOCK_STREAM ,0);
    // /api/v1/dummy
    //char *compute_get_request(char *host, char *url, char *query_params,
                            //char **cookies, int cookies_count)
    message = compute_get_request("3.248.203.233:8080", "/api/v1/dummy", NULL, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s \n", response);
    
    free(message);
    free(response);
    close_connection(sockfd);

    char** body_data = calloc(1, sizeof(char *));
    body_data[0] = calloc(1, 15);
    memcpy(body_data[0], "dummy=valoare", 14); // Am scos \0, 13 caractere + 1 null automat = 14
    
    // Ex 1.2: POST dummy and print response from main server
    //char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
       //                     int body_data_fields_count, char **cookies, int cookies_count)
    sockfd = open_connection("3.248.203.233", 8080, AF_INET, SOCK_STREAM ,0);
    message = compute_post_request("3.248.203.233:8080", "/api/v1/dummy", "application/x-www-form-urlencoded", body_data, 1, NULL, 0);
    printf("REQUEST POST:\n%s\n", message);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s \n", response);
    free(message);
    free(response);
    free(body_data[0]);
    free(body_data);
    close_connection(sockfd);
    
    // Ex 2: Login into main server
    body_data = calloc(1, sizeof(char *));
    sockfd = open_connection("3.248.203.233", 8080, AF_INET, SOCK_STREAM ,0);
    body_data[0] = calloc(1, 37);
    memcpy(body_data[0], "username=student&password=student", 33);
    message = compute_post_request("3.248.203.233:8080", "/api/v1/auth/login", "application/x-www-form-urlencoded", body_data, 1, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s \n", response);
    free(message);
    free(response);
    free(body_data[0]);
    free(body_data);
    close_connection(sockfd);


    // Ex 3: GET weather key from main server
    // Hardcodam Cookie-ul extras in Ex 2
    char *cookies[1];
    cookies[0] = "connect.sid=s%3Agb5hSqBZlW1ePKBffrxfn2ph09nBhzsB.aHHcwHlA%2BCwuugX9sZxNhwJWw9tJLFEX77HY4q2%2FSJQ"; 
    
    sockfd = open_connection("3.248.203.233", 8080, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request("3.248.203.233:8080", "/api/v1/weather/key", NULL, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s \n", response);
    
    // Extragem cheia folosind functia din helpers
    char weather_key[100] = {0};
    char *json_key = basic_extract_json_response(response);
    if (json_key) sscanf(json_key, "{\"key\":\"%[^\"]\"}", weather_key);
    
    free(message);
    free(response);
    close_connection(sockfd);


    // Ex 4: GET weather data from OpenWeather API
    // Aflam IP-ul pentru api.openweathermap.org
    struct hostent *hw = gethostbyname("api.openweathermap.org");
    char *owm_ip = inet_ntoa(*(struct in_addr *)hw->h_addr);
    
    sockfd = open_connection(owm_ip, 80, AF_INET, SOCK_STREAM, 0);
    
    char query_params[200];
    sprintf(query_params, "lat=44.7398&lon=22.2767&appid=%s", weather_key);
    
    message = compute_get_request("api.openweathermap.org", "/data/2.5/weather", query_params, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s \n", response);

    // Extragem JSON-ul cu vremea ca sa il trimitem la Ex 5
    char *weather_json = basic_extract_json_response(response);
    char *weather_body[1];
    weather_body[0] = weather_json; 
    
    close_connection(sockfd); // Nu dam free(response) inca, avem nevoie de weather_json in Ex 5


    // Ex 5: POST weather data for verification to main server
    sockfd = open_connection("3.248.203.233", 8080, AF_INET, SOCK_STREAM, 0);
    message = compute_post_request("3.248.203.233:8080", "/api/v1/weather/44.7398/22.2767", "application/json", weather_body, 1, cookies, 1);
    send_to_server(sockfd, message);
    
    char *response_ex5 = receive_from_server(sockfd); 
    printf("%s \n", response_ex5);
    
    free(message);
    free(response_ex5);
    free(response); // Acum putem elibera si response-ul de la Ex 4
    close_connection(sockfd);


    // Ex 6: Logout from main server
    sockfd = open_connection("3.248.203.233", 8080, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request("3.248.203.233:8080", "/api/v1/auth/logout", NULL, cookies, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s \n", response);
    
    free(message);
    free(response);
    close_connection(sockfd);

    // BONUS: make the main server return "Already logged in!"

    // free the allocated data at the end!

    return 0;
}