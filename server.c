#include "server.h"

// 创建读写锁
pthread_rwlock_t rwlock;

int main()
{
    // 避免写饥饿，将写优先级设置高于读优先级
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setkind_np(&attr,PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(&rwlock,&attr);

    // 资源初始化
    init();

    //创建|服务端和客户端地址
    struct sockaddr_in serv_addr, cli_addr;
    // 标记可用描述符下标
    int i;
    // 显示时间
    time_t timenow;
    // 创建专门接收终端消息的线程
    pthread_t thread;
    // 
    char buff[BUFFSIZE];

    printf("Running...\nEnter command \"quit\" to exit server.\n\n");

    bzero(&serv_addr, sizeof(struct sockaddr_in));  // 清零
    serv_addr.sin_family = AF_INET;                 // 协议
    serv_addr.sin_port = htons(PORT);               // 端口号8888
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // ip地址任意

    // 创建一个socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        perror("fail to socket");
        exit(-1);
    }
    // 绑定地址
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("fail to bind");
        exit(-2);
    }
    // 监听
    listen(listenfd, MAXMEM);

    // 创建一个线程，专门接收用户输入对服务器程序进行管理，调用quit函数
    pthread_create(&thread, NULL, (void *)(quit), NULL);

    // 将套接字描述符数组初始化为-1，表示空闲
    for (i = 0; i < MAXMEM; i++)
        connfd[i] = -1;

    while (1)
    {
        int len;
        // 寻找空闲套接字描述符
        for (i = 0; i < MAXMEM; i++)
        {
            if (connfd[i] == -1)
                break;
        }
        // accept 从listen接受的连接队列中取得一个连接
        connfd[i] = accept(listenfd, (struct sockaddr *)&cli_addr, &len);
        if (connfd[i] < 0) 
        {
            perror("fail to accept.");
        }

        // 显示时间
        timenow = time(NULL);
        printf("%.24s\n\tconnect from: %s, port %d\n",
               ctime(&timenow), inet_ntop(AF_INET, &(cli_addr.sin_addr), buff, BUFFSIZE),
               ntohs(cli_addr.sin_port));

        // 针对当前套接字创建一个线程，对当前套接字的消息进行处理
        pthread_create(malloc(sizeof(pthread_t)), NULL, (void *)(&rcv_snd), (void *)i);
    }
    return 0;
}

/*服务器接收和发送函数*/
void rcv_snd(int n)
{
    ssize_t len;
    int i;
    char mytime[32], buf[BUFFSIZE];
    char temp[BUFFSIZE];//存放临时数据
    char command[20], arg1[20], arg2[BUFFSIZE];
    time_t timenow;

    // 登录与注册循环
    while (1)
    {
        len = read(connfd[n], buf, BUFFSIZE);
        if (len > 0)
        {
            buf[len - 1] = '\0'; // 去除换行符

            if (strcmp(buf, "login") == 0)
            {
                //登录成功时退出该循环
                if (user_login(n) == 0)
                {
                    break;
                }
            }
            else if (strcmp(buf, "register") == 0)
            {
                // 注册功能
                register_user(n);
            }
            else if (strcmp(buf, "quit") == 0)
            {
                // 退出功能
                quit_client(n);
            }
        }
    }

    while (1)
    {
        if ((len = read(connfd[n], temp, BUFFSIZE)) > 0)
        {
            temp[len - 1] = '\0';
            sscanf(temp, "%s %s %[^\n]", command, arg1, arg2); //解析命令

            /*根据解析出的命令执行不同的函数*/
            if (strcmp(command, "send") == 0 && strcmp(arg1, "-all") == 0)
            {
                send_all_msg(arg2, n);// 消息 发送者
            }
            else if (strcmp(command, "send") == 0 && strcmp(arg1, "-chatroom") == 0)
            {
                send_chatroom_msg(arg2, n);// 消息 发送者
            }
            else if (strcmp(command, "send") == 0)
            {
                send_private_msg(arg1, arg2, n);// 接收者名称 消息 发送者
            }
            else if (strcmp(command, "quit") == 0)
            {
                quit_client(n);// 在线者
            }
            else if (strcmp(command, "chgpsw") == 0)
            {
                change_passwd(n, arg1);// 用户 密码
            }
            else if (strcmp(command, "create") == 0)
            {
                create_chatroom(arg1, arg2, n);// 名称 密码 创建者
            }
            else if (strcmp(command, "join") == 0)
            {
                join_chatroom(arg1, arg2, n);// 聊天室名称 密码 用户
            }
            else if (strcmp(command, "publish") == 0 && strcmp(arg1, ":") == 0)
            {
                publish_status(arg2,n);// 消息 用户
            }
            else if (strcmp(command, "view") == 0 && strcmp(arg1, ":") == 0)
            {
                view_status(n);// 用户
            }
            else if (strcmp(command, "ls") == 0 && strcmp(arg1, "-chatrooms") == 0)
            {
                get_online_chatrooms(n);// 用户
            }
            else if (strcmp(command, "ls") == 0 && strcmp(arg1, "-users") == 0)
            {
                get_online_users(n);// 用户
            }
            else if (strcmp(command, "ls") == 0 && strcmp(arg1, "-inrmusr") == 0)
            {
                get_inroom_users(n);// 用户
            }
            else if (strcmp(command, "exit") == 0)
            {
                exit_chatroom(n);// 用户
            }
            else
            {
                invalid_command(n);
            }
        }
    }
}

/*初始化*/
void init()
{
    int i, j;
    user_count = 0;
    chatroom_count = 0;
    statue_count = 0;
    for (i = 0; i < MAXMEM; i++)
    {
        online_users[i].status = -1;
    }
    for (i = 0; i < MAXROOM; i++)
    {
        chatrooms[i].status = -1;
        for (j = 0; j < 10; j++)
        {
            chatrooms[i].user[j] = -1;
        }
    }
    for (i = 0; i < BUFFSIZE; i++)
    {
        statues[i].status = -1;
    }
    char buf1[20];
    char buf2[20];
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;

    pthread_rwlock_rdlock(&rwlock);// 加锁

    fp1 = fopen("users.txt", "r");
    fp2 = fopen("statues.txt", "r");
    //从文件中读取用户
    while (fscanf(fp1, "%s", buf1) != EOF)
    {
        strcpy(users[user_count].username, buf1);
        fscanf(fp1, "%s", buf1);
        strcpy(users[user_count].password, buf1);
        user_count++;
    }
    //从文件中读取空间信息
    while (fscanf(fp2, "%s", buf2) != EOF)
    {
        strcpy(statues[statue_count].time, buf2);
        fscanf(fp2, "%s", buf2);
        strcpy(statues[statue_count].name, buf2);
        fscanf(fp2, "%s", buf2);
        strcpy(statues[statue_count].message, buf2);
        statues[statue_count].status=0;
        statue_count++;
    }
    fclose(fp2);
    fclose(fp1);

    pthread_rwlock_unlock(&rwlock);// 解锁
}

/*将用户保存到文件*/
void save_users()
{
    int i;
    char buf[20];
    FILE *fp = NULL;

    pthread_rwlock_wrlock(&rwlock);// 加锁

    fp = fopen("users.txt", "w+");
    for (i = 0; i < user_count; i++)
    {
        strcpy(buf, users[i].username);
        strcat(buf, "\n");
        fprintf(fp, buf);
        strcpy(buf, users[i].password);
        strcat(buf, "\n");
        fprintf(fp, buf);
    }
    fclose(fp);
    pthread_rwlock_unlock(&rwlock);// 解锁
}

/*将动态保存到文件*/
void save_statues()
{
    int i;
    char buf[256];
    FILE *fp = NULL;
    pthread_rwlock_wrlock(&rwlock);// 加锁
    fp = fopen("statues.txt", "w+");
    for (i = 0; i < statue_count; i++)
    {
        strcpy(buf, statues[i].time);
        strcat(buf, "\n");
        fprintf(fp, buf);
        strcpy(buf, statues[i].name);
        strcat(buf, "\n");
        fprintf(fp, buf);
        strcpy(buf, statues[i].message);
        strcat(buf, "\n");
        fprintf(fp, buf);
        statues[i].status=-1;
    }
    statue_count=0;
    fclose(fp);
    pthread_rwlock_unlock(&rwlock);// 解锁
}

/*服务器处理用户退出*/
void quit_client(int n)
{
    int ret, i;
    pthread_rwlock_wrlock(&rwlock);// 加锁
    // 关闭对应描述符
    close(connfd[n]);
    connfd[n] = -1;

    // 将在线用户设为离线
    for (i = 0; i < MAXMEM; i++)
    {
        if (n == online_users[i].socketfd)
        {
            online_users[i].status = -1;
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁
    pthread_exit(&ret);
}

/*用户登录*/
int user_login(int n)
{
    int len, i, j;
    char buf[BUFFSIZE], username[20], password[20];

    // 账号
    sprintf(buf, "your username: ");
    write(connfd[n], buf, strlen(buf) + 1);

    len = read(connfd[n], username, 20);
    if (len > 0)
    {
        username[len - 1] = '\0'; // 去除换行符
    }

    // 密码
    sprintf(buf, "your password: ");
    write(connfd[n], buf, strlen(buf) + 1);

    len = read(connfd[n], password, 20);
    if (len > 0)
    {
        password[len - 1] = '\0'; // 去除换行符
    }

    for (i = 0; i < MAXMEM; i++)
    {
        if (strcmp(username, users[i].username) == 0)
        {
            if (strcmp(password, users[i].password) == 0)
            {
                sprintf(buf, "Login successfully.\n\n");
                write(connfd[n], buf, strlen(buf + 1));
                
                pthread_rwlock_rdlock(&rwlock);
                // 找到空位
                for (j = 0; j < MAXMEM; j++)
                {
                    if (online_users[j].status == -1)
                        break;
                }
                strcpy(online_users[j].username, username);
                online_users[j].socketfd = n;
                online_users[j].status = 0;
                pthread_rwlock_unlock(&rwlock);// 解锁
                return 0;
            }
            else
            {
                sprintf(buf, "Wrong password.\n\n");
                write(connfd[n], buf, strlen(buf + 1));
                return -1;
            }
        }
    }
    sprintf(buf, "Account does not exist.\n\n");
    write(connfd[n], buf, strlen(buf + 1));
    return -1;
}

/*用户注册*/
void register_user(int n)
{
    int len, i;
    char buf[BUFFSIZE], username[20], password[20];

    sprintf(buf, "your username: ");
    write(connfd[n], buf, strlen(buf) + 1);

    len = read(connfd[n], username, 20);
    if (len > 0)
    {
        username[len - 1] = '\0'; // 去除换行符
    }

    sprintf(buf, "your password: ");
    write(connfd[n], buf, strlen(buf) + 1);

    len = read(connfd[n], password, 20);
    if (len > 0)
    {
        password[len - 1] = '\0'; // 去除换行符
    }

    pthread_rwlock_rdlock(&rwlock);// 读锁
    // 寻找是否有重复用户名
    for (i = 0; i < MAXMEM; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            strcpy(buf, "The username already exists.\n\n");
            write(connfd[n], buf, strlen(buf) + 1);
            return;
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁

    pthread_rwlock_wrlock(&rwlock);
    // 将新注册的用户添加
    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    user_count++;
    pthread_rwlock_unlock(&rwlock);// 解锁

    sprintf(buf, "Account created successfully.\n\n");
    write(connfd[n], buf, strlen(buf) + 1);
    
}

/*用户发送私聊信息*/
void send_private_msg(char *username, char *data, int sfd)
{
    int i, j;
    time_t now;
    char send_man[20];
    char buf[BUFFSIZE], nowtime[20], temp[30];
    // 获取当前时间
    now = time(NULL);
    time(&now);
    struct tm *tempTime = localtime(&now);
    strftime(nowtime, 20, "[%H:%M:%S]", tempTime);
    // 发送方名称
    for (j = 0; j < MAXMEM; j++)
    {
        if (sfd == online_users[j].socketfd)
        {
            strcpy(send_man, online_users[j].username);
            break;
        }
    }
    pthread_rwlock_rdlock(&rwlock);
    // 寻找接收者是否在线
    for (i = 0; i < MAXMEM; i++)
    {
        if (strcmp(username, online_users[i].username) == 0)
        {
            // 打包发送
            strcpy(buf, nowtime);
            strcat(buf, "\t");
            strcat(buf, "from ");
            strcat(buf, send_man);
            strcat(buf, ":\n");
            strcat(buf, data);
            strcat(buf, "\n");
            write(connfd[online_users[i].socketfd], buf, strlen(buf) + 1);
            strcpy(temp, "Sent successfully.\n");
            write(connfd[sfd], temp, strlen(temp) + 1);
            pthread_rwlock_unlock(&rwlock);// 解锁
            return;
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁
    strcpy(buf, "User is not online or user does not exist.\n");
    write(connfd[sfd], buf, strlen(buf) + 1);
    return;
}

/*用户群发信息给所有用户*/
void send_all_msg(char *msg, int sfd)
{
    int i;
    char buf[BUFFSIZE], nowtime[20], send_man[20], temp[30];

    // 获取时间
    time_t now;
    time(&now);
    struct tm *tempTime = localtime(&now);
    strftime(nowtime, 20, "[%H:%M:%S]", tempTime);
    
    // 获取发送方用户名
    for (i = 0; i < MAXMEM; i++)
    {
        if (sfd == online_users[i].socketfd)
        {
            strcpy(send_man, online_users[i].username);
            break;
        }
    }
    
    // 打包消息
    strcpy(buf, nowtime);
    strcat(buf, "\t");
    strcat(buf, "from ");
    strcat(buf, send_man);
    strcat(buf, "(goup-sent):\n");
    strcat(buf, msg);
    strcat(buf, "\n");

    pthread_rwlock_rdlock(&rwlock);
    for (i = 0; i < MAXMEM; i++)
    {
        // 存在且不是自己
        if (connfd[i] != -1 && i != sfd)
        {
            write(connfd[i], buf, strlen(buf) + 1);
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁
    strcpy(temp, "Sent successfully\n");
    write(connfd[sfd], temp, strlen(temp) + 1);
}

/*获取所有在线用户信息*/
void get_online_users(int sfd)
{
    int i;
    char buf[BUFFSIZE], nowtime[20];
    time_t now;

    time(&now);
    struct tm *tempTime = localtime(&now);
    strftime(nowtime, 20, "[%H:%M:%S]", tempTime);
    strcpy(buf, nowtime);
    strcat(buf, "\t");
    strcat(buf, "All online user(s):\n");

    pthread_rwlock_rdlock(&rwlock);
    for (i = 0; i < MAXMEM; i++)
    {
        if (online_users[i].status == 0)
        {
            strcat(buf, "\t");
            strcat(buf, online_users[i].username);
            strcat(buf, "\n");
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁
    write(connfd[sfd], buf, strlen(buf) + 1);
}

/*向聊天室发送信息*/
void send_chatroom_msg(char *msg, int sfd)
{
    int i, j, k;
    int flag;
    flag = -1;
    char buf[BUFFSIZE], nowtime[20];
    // 获取时间
    time_t now;
    time(&now);
    struct tm *tempTime = localtime(&now);
    strftime(nowtime, 20, "[%H:%M:%S]", tempTime);
    // 在聊天室中寻找发送者
    for (i = 0; i < MAXROOM; i++)
    {
        if (chatrooms[i].status == 0)
        {
            for (j = 0; j < 10; j++)
            {
                if (chatrooms[i].user[j] == sfd)
                {
                    flag = 0;
                    break;
                }
            }
        }
        if (flag == 0)
        {
            break;
        }
    }
    if (flag == -1)
    {
        strcpy(buf, "You have not joined the chat room.\n");
        write(connfd[sfd], buf, strlen(buf) + 1);
    }
    else
    {
        // 寻找发送者用户名
        for (k = 0; k < MAXMEM; k++)
        {
            if (online_users[k].status == 0 && online_users[k].socketfd == sfd)
                break;
        }
        // 打包消息
        strcpy(buf, nowtime);
        strcat(buf, "\tchatroom ");
        strcat(buf, chatrooms[i].name);
        strcat(buf, ":\nfrom ");
        strcat(buf, online_users[k].username);
        strcat(buf, ":\t");
        strcat(buf, msg);
        strcat(buf, "\n");
        pthread_rwlock_rdlock(&rwlock);
        // 发送消息
        for (k = 0; k < 10; k++)
        {
            if (chatrooms[i].user[k] != -1)
            {
                write(connfd[chatrooms[i].user[k]], buf, strlen(buf) + 1);
            }
        }
        pthread_rwlock_unlock(&rwlock);// 解锁
    }
}

/*创建聊天室*/
void create_chatroom(char *name, char *passwd, int sfd)
{
    int i, j;
    char buf[BUFFSIZE];

    pthread_rwlock_wrlock(&rwlock);
    // 寻找空聊天室
    for (i = 0; i < MAXROOM; i++)
    {
        if (chatrooms[i].status == -1)
            break;
    }
    strcpy(chatrooms[i].name, name);
    strcpy(chatrooms[i].passwd, passwd);
    chatrooms[i].status = 0;

    // 将创建者加入聊天室
    for (j = 0; j < 10; j++)
    {
        if (chatrooms[i].user[j] == -1)
            break;
    }
    chatrooms[i].user[j] = sfd;
    pthread_rwlock_unlock(&rwlock);// 解锁

    strcpy(buf, "Successfully created chat room.\n");
    write(connfd[sfd], buf, strlen(buf) + 1);
}

/*加入聊天室*/
void join_chatroom(char *name, char *passwd, int sfd)
{
    int i, j;
    int room, flag;
    char buf[BUFFSIZE];
    flag = -1;
    // 判断是否已经加入聊天室
    for (i = 0; i < MAXROOM; i++)
    {
        for (j = 0; j < 10; j++)
        {
            if (chatrooms[i].user[j] == sfd)
            {
                room = i;
                flag = 0;
            }
        }
    }
    if (flag == 0)
    {
        strcpy(buf, "You have joined the chat room ");
        strcat(buf, chatrooms[room].name);
        strcat(buf, ".\n");
        write(connfd[sfd], buf, strlen(buf) + 1);
    }
    else
    {
        for (i = 0; i < MAXROOM; i++)
        {
            if (chatrooms[i].status != -1)// 状态在线
            {
                if (strcmp(chatrooms[i].name, name) == 0)// 名称正确
                {
                    if (strcmp(chatrooms[i].passwd, passwd) == 0)// 密码正确
                    {
                        pthread_rwlock_wrlock(&rwlock);
                        for (j = 0; j < 10; j++)
                        {
                            if (chatrooms[i].user[j] == -1)// 寻找空余
                            {
                                break;
                            }
                        }
                        // 加入
                        chatrooms[i].user[j] = sfd;
                        pthread_rwlock_unlock(&rwlock);// 解锁
                        strcpy(buf, "Successfully joined the chat room.\n");
                        write(connfd[sfd], buf, strlen(buf) + 1);
                        return;
                    }
                    else
                    {
                        strcpy(buf, "Incorrect chat room password.\n");
                        write(connfd[sfd], buf, strlen(buf) + 1);
                        return;
                    }
                }
            }
        }
    }
}

/*发表动态*/
void publish_status(char *msg, int sfd)
{
    int j;
    char buf[BUFFSIZE],nowtime[20],name[20];
    // 获取时间
    time_t now;
    time(&now);
    struct tm *tempTime = localtime(&now);
    strftime(nowtime, 20, "[%H:%M:%S]", tempTime);
    strcpy(buf,nowtime);
    // 获取姓名
    for (j = 0; j < MAXMEM; j++)
    {
        if (sfd == online_users[j].socketfd)
        {
            strcpy(name, online_users[j].username);
            break;
        }
    }

    pthread_rwlock_wrlock(&rwlock);
    // 将消息写入空间数组
    strcpy(statues[statue_count].name,name);
    strcpy(statues[statue_count].time,nowtime);
    strcpy(statues[statue_count].message,msg);
    statues[statue_count].status=0;
    statue_count++;

    // 如果空间不够，将数组中 status 值为 0 的数据移动到数组低位，并将高位空出来
    if(statue_count >= BUFFSIZE){
        int write_index = 0;  
        for (int read_index = 0; read_index < BUFFSIZE; read_index++) {  
            if (statues[read_index].status == 0) {  
                statues[write_index] = statues[read_index];  
                write_index++;  
            }  
        }
        statue_count = write_index;
    }

    if(statue_count <= BUFFSIZE){
        // 发送发表成功
        bzero(buf, sizeof(buf));  // 清零
        strcpy(buf, "\npublish statue succeed:\n");
        write(connfd[sfd], buf, strlen(buf) + 1);

        bzero(buf, sizeof(buf));  // 清零
        strcpy(buf,"\n========================\n");
        strcat(buf,statues[statue_count-1].time);
        strcat(buf," publish\n");
        strcat(buf,statues[statue_count-1].name);
        strcat(buf,":\n\t");
        strcat(buf,statues[statue_count-1].message);
        strcat(buf,"\n========================\n");
        write(connfd[sfd], buf, strlen(buf) + 1);
    }else
    {
        // 发送发表失败
        bzero(buf, sizeof(buf));  // 清零
        strcpy(buf, "publish statue false!\n");
        write(connfd[sfd], buf, strlen(buf) + 1);
    }
    pthread_rwlock_unlock(&rwlock);// 解锁
}

/*查看动态*/
void view_status(int sfd)
{
    int i;
    char buf[BUFFSIZE],nowtime[20],name[20];

    pthread_rwlock_rdlock(&rwlock);
    for(i=0; i<BUFFSIZE; i++){
        // 拼接消息
        if (statues[i].status == 0)
        {
            bzero(buf, sizeof(buf));  // 清零
            strcpy(buf,"\n========================\n");
            strcat(buf,statues[i].time);
            strcat(buf," publish\n");
            strcat(buf,statues[i].name);
            strcat(buf,":\n\t");
            strcat(buf,statues[i].message);
            strcat(buf,"\n========================\n");
            write(connfd[sfd], buf, strlen(buf) + 1);
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁

    bzero(buf, sizeof(buf));  // 清零
    strcpy(buf,"end\n\n");
    write(connfd[sfd], buf, strlen(buf) + 1);
}

/*获取所有已创建的聊天室的信息*/
void get_online_chatrooms(int sfd)
{
    int i;
    char buf[BUFFSIZE], nowtime[20];

    time_t now;
    time(&now);
    struct tm *tempTime = localtime(&now);
    strftime(nowtime, 20, "[%H:%M:%S]", tempTime);
    strcpy(buf, nowtime);
    strcat(buf, "\tAll online chat room(s):\n");

    pthread_rwlock_rdlock(&rwlock);
    for (i = 0; i < MAXROOM; i++)
    {
        if (chatrooms[i].status == 0)
        {
            strcat(buf, "\t");
            strcat(buf, chatrooms[i].name);
            strcat(buf, "\n");
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁

    write(connfd[sfd], buf, strlen(buf) + 1);
}

/*修改密码*/
void change_passwd(int sfd, char *passwd)
{
    int i, j;
    char buf[BUFFSIZE], name[20];
    // 获取用户名
    for (j = 0; j < MAXMEM; j++)
    {
        if (sfd == online_users[j].socketfd)
        {
            strcpy(name, online_users[j].username);
            break;
        }
    }
    pthread_rwlock_wrlock(&rwlock);
    // 根据用户名修改密码
    for (i = 0; i < MAXMEM; i++)
    {
        if (strcmp(name, users[i].username) == 0)
        {
            strcpy(users[i].password, passwd);
            strcpy(buf, "Password has been updated.\n");
            write(connfd[sfd], buf, strlen(buf) + 1);
            break;
        }
    }
    pthread_rwlock_unlock(&rwlock);// 解锁
}

/*查询所有加入某聊天室的用户*/
void get_inroom_users(int sfd)
{
    int i, j;
    int room, flag; //room记录查询查询发起人所在的房间，flag标识用户是否加入房间
    flag = -1;
    char buf[BUFFSIZE], nowtime[20];

    time_t now;
    time(&now);
    struct tm *tempTime = localtime(&now);
    strftime(nowtime, 20, "[%H:%M:%S]", tempTime);

    for (i = 0; i < MAXROOM; i++)
    {
        for (j = 0; j < 10; j++)
        {
            if (chatrooms[i].user[j] == sfd)
            {
                room = i;
                flag = 0;
            }
        }
    }
    if (flag == -1)
    {
        strcpy(buf, "Sorry, you have not joined the chat room.\n");
        write(connfd[sfd], buf, strlen(buf) + 1);
    }
    else
    {
        strcpy(buf, nowtime);
        strcat(buf, "\tAll users in the ");
        strcat(buf, chatrooms[room].name);
        strcat(buf, ":\n");

        pthread_rwlock_rdlock(&rwlock);
        for (i = 0; i < 10; i++)
        {
            if (chatrooms[room].user[i] >= 0)
                for (j = 0; j < MAXMEM; j++)
                {
                    if (online_users[j].status != -1 && (chatrooms[room].user[i] == online_users[j].socketfd))
                    {
                        strcat(buf, "\t");
                        strcat(buf, online_users[j].username);
                        strcat(buf, "\n");
                    }
                }
        }
        pthread_rwlock_unlock(&rwlock);// 解锁
        write(connfd[sfd], buf, strlen(buf) + 1);
    }
}

/*退出聊天室*/
void exit_chatroom(int sfd)
{
    int i, j;
    int room, flag;
    flag = -1;
    char buf[BUFFSIZE];
    
    pthread_rwlock_wrlock(&rwlock);
    for (i = 0; i < MAXROOM; i++)
    {
        if (chatrooms[i].status == 0)
        {
            for (j = 0; j < 10; j++)
            {
                if (chatrooms[i].user[j] == sfd)
                {
                    chatrooms[i].user[j] = -1;
                    room = i;
                    flag = 0;
                    break;
                }
            }
        }
        if (flag == 0)
            break;
    }
    pthread_rwlock_unlock(&rwlock);// 解锁

    if (flag == -1)
    {
        strcpy(buf, "You have not joined the chat room.\n");
        write(connfd[sfd], buf, strlen(buf) + 1);
    }
    else
    {
        strcpy(buf, "Successfully quit chat room ");
        strcat(buf, chatrooms[room].name);
        strcat(buf, ".\n");
        write(connfd[sfd], buf, strlen(buf) + 1);
    }
}

/*服务器推出*/
void quit()
{
    int i;
    char msg[10];
    while (1)
    {
        scanf("%s", msg); // scanf 不同于fgets, 它不会读入最后输入的换行符
        if (strcmp(msg, "quit") == 0)
        {
            save_users();
            save_statues();
            printf("Byebye... \n");

            //销毁读写锁
            pthread_rwlock_destroy(&rwlock);
            close(listenfd);
            exit(0);
        }
    }
}

/*用户输入无效命令*/
void invalid_command(int sfd)
{
    char buf[BUFFSIZE];
    strcpy(buf, "Invalid command.\n");
    write(connfd[sfd], buf, strlen(buf) + 1);
}
