#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#define SIZE 1024

char *ip = "127.0.0.1";
char *files[] = {"building1.jpg", "building2.jpg", "footballField.jpg"};

void udp_send_file(FILE *fp, int sockfd, struct sockaddr_in addr)
{
  int n;
  char buffer[SIZE];

  // Sending the data
  while (fread(&buffer, SIZE, 1, fp) > 0)
  {
    if (sendto(sockfd, buffer, SIZE, 0, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
      perror("[-] sending data to the server.\n");
      exit(1);
    }
    bzero(buffer, SIZE);
  }
  bzero(buffer, SIZE);
  // Sending the 'END'
  strcpy(buffer, "END");
  strcat(buffer, "\n");

  if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    perror("[-] Error sending end of file \n");
  }
}

void tcp_send_file(FILE *fp, int sockfd)
{
  int n;
  char data[SIZE] = {0};

  while (fread(&data, SIZE, 1, fp) > 0)
  {
    if (send(sockfd, data, sizeof(data), 0) == -1)
    {
      perror("[-]Error in sending file. \n");
      exit(1);
    }

    bzero(data, SIZE);
  }

  bzero(data, SIZE);

  // Sending the 'END'
  strcpy(data, "END");
  strcat(data, "\n");
  if (send(sockfd, data, sizeof(data), 0) == -1)
  {
    perror("[-] Error sending end of file \n");
  }
  /*
  bzero(data, SIZE);
  sleep(1);
  if (recv(sockfd, data, SIZE, 0) <= 0)
  {
    perror("[-] Failed to get reply from server. \n");
  }
  else
  {
    printf("%s", data);
  }
  */
}

int create_socket(int port, int type)
{
  int sockfd = socket(AF_INET, type, 0);
  printf("Socket created ... ");
  if (sockfd < 0)
  {
    perror("[-]Error in socket\n");
    strerror(errno);
    exit(1);
  }

  return sockfd;
}

struct sockaddr_in get_server_address(int port)
{
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(ip);

  return server_addr;
}

void tcp_connect(int sockfd, struct sockaddr_in server_addr)
{
  int e = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (e == -1)
  {
    perror("[-]Error in socket");
    exit(1);
  }

  printf("[+]Connected to Server.\n");
}

FILE *open_file(char *filename, char *mode)
{
  FILE *fp = fopen(filename, mode);
  if (fp == NULL)
  {
    perror("[-]Error in reading file. \n");
    exit(1);
  }

  return fp;
}

int get_socket(int port, int socket_type)
{
  int sockfd = create_socket(port, socket_type);

  if (SOCK_STREAM == socket_type)
  {
    struct sockaddr_in server_addr = get_server_address(port);
    tcp_connect(sockfd, server_addr);
  }

  return sockfd;
}

int main()
{
  int tcp_port = 8081;
  int udp_port = 8082;

  char filename[25];
  int option;

  printf("1. TCP \n");
  printf("2. UDP \n");
  printf("Please select option to send file : \n");
  scanf("%d", &option);

  int socket_type;
  int sockfd;
  struct sockaddr_in server_addr;

  if (1 == option)
  {
    sockfd = get_socket(tcp_port, SOCK_STREAM);
    printf("Sending on TCP \n");
    int number_of_files = sizeof(files) / sizeof(char *);
    printf("Number of files : %d\n", number_of_files);
    for (int i = 0; i < number_of_files; i++)
    {
      sprintf(filename, "img/%s", files[i]);
      printf("Sending file %s\n", filename);
      FILE *fp = open_file(filename, "rb");
      tcp_send_file(fp, sockfd);
      fclose(fp);
    }
    printf("Sent %d files", number_of_files);
  }
  else
  {
    sockfd = get_socket(udp_port, SOCK_DGRAM);
    server_addr = get_server_address(udp_port);
    printf("Sending on UDP \n");
    int number_of_files = sizeof(files) / sizeof(char *);
    printf("Number of files : %d\n", number_of_files);
    for (int i = 0; i < number_of_files; i++)
    {
      sprintf(filename, "img/%s", files[i]);
      FILE *fp = open_file(filename, "rb");
      udp_send_file(fp, sockfd, server_addr);
      fclose(fp);
    }
    printf("Sent %d files", number_of_files);
  }

  printf("[+]File data sent successfully.\n");

  printf("[+]Closing the connection.\n");

  close(sockfd);

  return 0;
}
