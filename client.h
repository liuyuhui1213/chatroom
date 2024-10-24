#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFSIZE 128
#define HOST_IP "192.168.1.195"
#define PORT 8888

int sockfd;
void snd();
void get_help();
