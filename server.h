#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

/*存储用户*/
struct user
{
    char username[20];
    char password[20];
};

/*存储用户及其用户套接字文件描述符*/
struct user_socket
{
    char username[20];
    int socketfd;
    int status; //标识是否在线 0:在线 -1:下线
};

/*存储聊天室*/
struct chatroom
{
    char name[20];
    char passwd[20];
    int user[10]; //加入聊天室的人的fd
    int status;   //标识是否还存在 0:存在 -1:销毁
};

/*存储空间消息*/
struct statue
{
    char name[20];
    char time[20];
    char message[256];
    int status;
};

#define PORT 8888
#define MAXMEM 20       // 最大在线人数
#define MAXROOM 5       // 最大在线房间数
#define BUFFSIZE 256    // 最大消息数、最大动态数

int user_count;     //记录总的用户数
int chatroom_count; //记录聊天室个数
int statue_count;   //记录动态个数
int listenfd;       //Socket描述符
int connfd[MAXMEM]; //和用户交互的套接字描述符
struct user users[MAXMEM];               //记录所有用户
struct user_socket online_users[MAXMEM]; //记录在线用户
struct chatroom chatrooms[MAXROOM];      //记录聊天室
struct statue statues[BUFFSIZE];         //记录动态

void init();
void quit();
void save_users();
void save_statues();
void register_user(int n);
void rcv_snd(int p);
void quit_client(int n);
int user_login(int n);
void get_help();
void send_private_msg(char *username, char *data, int sfd);
void send_all_msg(char *msg, int sfd);
void get_online_users(int sfd);
void send_chatroom_msg(char *msg, int sfd);
void create_chatroom(char *name, char *passwd, int sfd);
void join_chatroom(char *name, char *passwd, int sfd);
void publish_status(char *msg, int sfd);
void view_status(int sfd);
void get_online_chatrooms(int sfd);
void change_passwd(int sfd, char *passwd);
void get_inroom_users(int sfd);
void exit_chatroom(int sfd);
void invalid_command(int sfd);
