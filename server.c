#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>

#define SIZE 1024

char *ip = "127.0.0.1";
int number_of_files = 3;

int is_end_command(char *line)
{
    if (4 == strlen(line))
    {
        char first_three[4];
        strncpy(first_three, line, 3);
        if (strstr(first_three, "END"))
            return 1;
    }
    return 0;
}

int get_file_size(FILE *fp)
{
    struct stat st;
    int fd = fileno(fp); // get file descriptor
    fstat(fd, &st);
    off_t size = st.st_size;

    return size;
}

long write_file_from_udp_socket(int sockfd, struct sockaddr_in addr, char *filename)
{
    int n;
    char buffer[SIZE];
    socklen_t addr_size;
    int is_end_of_file = 0;

    // Creating a file.
    FILE *fp = fopen(filename, "wb");

    // Receiving the data and writing it into the file.
    while (1)
    {
        addr_size = sizeof(addr);
        n = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr *)&addr, &addr_size);
        if (1 == is_end_command(buffer))
        {
            printf("Received END of file \n");
            break;
        }

        if (n > 0)
        {
            fwrite(buffer, sizeof(buffer), 1, fp);
            bzero(buffer, SIZE);
        }
    }

    long bytes_received = ftell(fp);
    fclose(fp);
    printf("Received file : %s with size: %ld \n", filename, bytes_received);
    return bytes_received;
}

long write_files_from_tcp_socket(int sockfd, char *filename)
{
    int n;
    FILE *fp;

    char buffer[SIZE];

    fp = fopen(filename, "wb");
    // int is_end_of_file = 0;
    while (1)
    {
        n = recv(sockfd, buffer, SIZE, 0);
        if (n <= 0)
        {
            break;
        }
        if (n > 0 && 1 == is_end_command(buffer))
        {
            printf("Received END of file \n");
            break;
        }

        if (n > 0)
        {
            fwrite(buffer, sizeof(buffer), 1, fp);
            // fprintf(fp, "%s", buffer);
            bzero(buffer, SIZE);
        }
    }

    long bytes_received = ftell(fp);
    fclose(fp);
    printf("Received file : %s with size: %ld \n", filename, bytes_received);
    return bytes_received;
}

struct sockaddr_in get_server_address(int port)
{
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    return server_addr;
}

void socket_bind(struct sockaddr_in server_addr, int sockfd)
{
    int e = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (e < 0)
    {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");
}

void tcp_listen(struct sockaddr_in server_addr, int sockfd)
{
    if (listen(sockfd, 10) == 0)
    {
        printf("[+]Listening at port %d....\n", server_addr.sin_port);
    }
    else
    {
        perror("[-]Error in listening");
        exit(1);
    }
}

int tcp_connect(struct sockaddr_in server_addr, int sockfd)
{
    struct sockaddr_in new_addr;

    socklen_t addr_size = sizeof(new_addr);
    int new_socket = accept(sockfd, (struct sockaddr *)&new_addr, &addr_size);

    return new_socket;
}

int create_socket(char *ip, int port, int type)
{
    int sockfd = socket(AF_INET, type, 0);
    if (sockfd < 0)
    {
        perror("[-]Error in socket");
        exit(1);
    }

    return sockfd;
}

void *close_socket(void *args)
{
    int sockfd = *((int *)args);
    close(sockfd);
    return NULL;
}

void handle_tcp_connection(struct sockaddr_in server_addr, int sockfd)
{
    printf("In TCP handler");
    int newsockfd = tcp_connect(server_addr, sockfd);

    char buffer[SIZE];
    bzero(buffer, SIZE);

    char filename[20];
    long bytes_received = 0;
    for (int i = 0; i < number_of_files; i++)
    {
        sprintf(filename, "received/file_%d.jpg", i);
        printf("Receiving file %s", filename);
        bytes_received += write_files_from_tcp_socket(newsockfd, filename);
    }

    /*
sprintf(buffer, "Total bytes sent : %ld", bytes_received);
printf("%s", buffer);
if (send(sockfd, buffer, sizeof(buffer), 0) == -1)
{
    perror("Failed to send \n");
}

sleep(2);
printf("[+] Data written in the file successfully. \n");
*/
}

void handle_udp_connection(int sockfd)
{
    struct sockaddr_in addr;
    char filename[20];
    long bytes_received = 0;
    char buffer[SIZE];

    for (int i = 0; i < number_of_files; i++)
    {
        sprintf(filename, "received/file_%d.jpg", i);
        bytes_received += write_file_from_udp_socket(sockfd, addr, filename);
    }
    /*
    bzero(buffer, SIZE);
    sprintf(buffer, "Total bytes sent : %ld", bytes_received);
    printf("%s", buffer);

    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("Failed to send");
    }
    */
}

int main(int argc, char const *argv[])
{
    pthread_t tcp_thread;
    pthread_t udp_thread;

    int tcp_port = 8081;
    int udp_port = 8082;

    int tcp_socket = create_socket(ip, tcp_port, SOCK_STREAM);
    printf("[+] TCP server socket created successfully.\n");

    int udp_socket = create_socket(ip, udp_port, SOCK_DGRAM);
    printf("[+] UDP server socket created successfully.\n");

    struct sockaddr_in tcp_server_addr = get_server_address(tcp_port);
    struct sockaddr_in udp_server_addr = get_server_address(udp_port);

    socket_bind(tcp_server_addr, tcp_socket);
    tcp_listen(tcp_server_addr, tcp_socket);
    socket_bind(udp_server_addr, udp_socket);

    fd_set read_sockets, ready_sockets;

    FD_ZERO(&read_sockets);
    FD_SET(tcp_socket, &read_sockets);
    FD_SET(udp_socket, &read_sockets);

    ready_sockets = read_sockets;

    if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
    {
        perror("select failed");
        exit(1);
    }

    for (int i = 0; i < FD_SETSIZE; i++)
    {
        if (FD_ISSET(i, &ready_sockets))
        {
            if (i == tcp_socket)
            {
                printf("Test");
                handle_tcp_connection(tcp_server_addr, tcp_socket);
            }

            if (i == udp_socket)
            {

                handle_udp_connection(udp_socket);
            }
        }
    }

    return 0;
}
