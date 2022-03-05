/*
 * @Author: Yinwhe
 * @Date: 2021-12-19 10:39:35
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-12-19 19:37:52
 * @Description: Client part
 * @Copyright: Copyright (c) 2021
 */

#include "include.h"
using namespace std;

void *helper_thread(void *arg)
{
    int sockfd = *(int *)arg;

    Message msg;
    key_t key = ftok("c", 12);
    int msgID = msgget(key, IPC_CREAT | 0666);

    if (msgID < 0)
    {
        printf("[Error] Message queue create fail: %s", strerror(errno));
    }

    recv(sockfd, &msg.data, sizeof(msg.data), 0);
    printf("%s\n>", msg.data);

    while (1)
    {
        memset(&msg, 0, sizeof(msg));

        if (recv(sockfd, &msg, sizeof(msg), 0) < 0)
        {
            printf("[Error] helper recv fail, error: %s\n", strerror(errno));
        }

        if (msg.type == REPOST)
        {
            printf("Receive repost: %s\n", msg.data);
            continue;
        }

        msgsnd(msgID, &msg, MAX_SIZE, 0);
    }
}

class Client
{
public:
    Client()
    {
        sockfd = -1;
        memset(&server_addr, 0, sizeof(server_addr));

        key_t key = ftok("c", 12);
        msgID = msgget(key, IPC_CREAT | 0666);
        if (msgID < 0)
        {
            printf("[Error] Message queue create fail: %s\n", strerror(errno));
            return;
        }

        char msg[MAX_SIZE];
        while(msgrcv(msgID, &msg, MAX_SIZE, 0, IPC_NOWAIT) > 0);
    }

    ~Client()
    {
        close(sockfd);
    }

    int operation()
    {
        int op;
        printf("\nPlease choose from:\n"
               "1.connect\n"
               "2.disconnect\n"
               "3.get time\n"
               "4.get server name\n"
               "5.get client list\n"
               "6.send data to other client\n"
               "7.exit\n"
               "> ");
        scanf("%d", &op);
        return op;
    }

    void run()
    {
        while (1)
        {
            int op = operation();
            switch (op)
            {
            case 1:
                connectServer();
                break;
            case 2:
                disconnectServer();
                break;
            case 3:
                getServerTime();
                break;
            case 4:
                getServerName();
                break;
            case 5:
                getClientList();
                break;
            case 6:
                sendToOtherClient();
                break;
            case 7:
                done();
                break;
            default:
                printf("[Error] Invalid Op!\n");
            }
        }
    }

private:
    int sockfd, msgID;
    char sendline[MAX_SIZE], recvline[MAX_SIZE];
    sockaddr_in server_addr;
    pthread_t thread;

    void connectServer()
    {
        if (sockfd != -1)
        {
            printf("[Error] Reconnected!\n");
            return;
        }

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("[Error] Create socket fail, error: %s\n", strerror(errno));
            return;
        }

        char ip[MAX_SIZE];
        // printf("Please enter server ip:");
        // scanf("%s", ip);

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(3302);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            printf("[Error] Connect fail, error: %s\n", strerror(errno));
            return;
        }

        if (pthread_create(&thread, NULL, helper_thread, &sockfd) != 0)
        {
            printf("[Error] Create thread fails!\n");
        }

        printf("[Info] Connected!\n");

        return;
    }

    void disconnectServer()
    {
        if (sockfd == -1)
        {
            printf("[Error] No connect detected!\n");
            return;
        }

        char data = DISCONNECT;
        if (send(sockfd, &data, sizeof(data), 0) < 0)
        {
            printf("[Error] Disconnect send fail, error: %s\n", strerror(errno));
        }

        pthread_cancel(thread);
        close(sockfd);
        sockfd = -1;
        printf("[Info] Connection closed!\n");

        return;
    }

    void getServerTime()
    {

        if (sockfd == -1)
        {
            printf("[Error] No connection detected!\n");
            return;
        }

        char data = GET_TIME;
        if (send(sockfd, &data, sizeof(data), 0) < 0)
        {
            printf("[Error] getServerTime send fail, error: %s\n", strerror(errno));
            return;
        }

        Message msg;
        long type = GET_TIME;
        if (msgrcv(msgID, &msg, MAX_SIZE, type, 0) < 0)
        {
            printf("[Error] getServerTime recv fail, error: %s\n", strerror(errno));
            return;
        }

        // printf("%s", msg.data);

        time_t t;
        sscanf(msg.data, "%ld", &t);
        printf("Time: %s\n", ctime(&t));

        return;
    }

    void getServerName()
    {
        if (sockfd == -1)
        {
            printf("[Error] No connection detected!\n");
            return;
        }

        char data = GET_NAME;
        if (send(sockfd, &data, sizeof(data), 0) < 0)
        {
            printf("[Error] getServerName send fail, error: %s\n", strerror(errno));
            return;
        }

        Message msg;
        long type = GET_NAME;
        if (msgrcv(msgID, &msg, MAX_SIZE, type, 0) < 0)
        {
            printf("[Error] getServerName recv fail, error: %s\n", strerror(errno));
            return;
        }

        printf("Server Name: %s\n", msg.data);

        return;
    }

    void getClientList()
    {
        if (sockfd == -1)
        {
            printf("[Error] No connection detected!\n");
            return;
        }

        char data = GET_CLIENT_LIST;
        if (send(sockfd, &data, sizeof(data), 0) < 0)
        {
            printf("[Error] getClientList send fail, error: %s\n", strerror(errno));
            return;
        }

        Message msg;
        long type = GET_CLIENT_LIST;
        if (msgrcv(msgID, &msg, MAX_SIZE, type, 0) < 0)
        {
            printf("[Error] getClientList recv fail, error: %s\n", strerror(errno));
            return;
        }

        printf("Client list: %s\n", msg.data);

        return;
    }

    void sendToOtherClient()
    {
        if (sockfd == -1)
        {
            printf("[Error] No connection detected!\n");
            return;
        }

        Message msg;
        msg.type = SEND_MSG;
        char ip[MAX_SIZE];
        int port;

        printf("Target IP: ");
        scanf("%s", ip);
        printf("Target port: ");
        scanf("%d", &port);
        sprintf(msg.data, "%s:%d:", ip, port);

        char content[MAX_SIZE - 50];
        printf("Send content: ");
        scanf("%s", content);
        sprintf(msg.data + strlen(msg.data), "%s", content);
        printf("%s\n", msg.data);

        if (send(sockfd, &msg, sizeof(msg), 0) < 0)
        {
            printf("[Error] sendToOtherClient send fail, error: %s\n", strerror(errno));
            return;
        }

        memset(&msg, 0, sizeof(msg));
        long type = SEND_MSG;
        if (msgrcv(msgID, &msg, MAX_SIZE, type, 0) < 0)
        {
            printf("[Error] sendToOtherClient recv fail, error: %s\n", strerror(errno));
            return;
        }

        printf("Server Response: %s\n", msg.data);

        return;
    }

    void done()
    {
        if (sockfd != -1)
        {
            char data = DISCONNECT;
            send(sockfd, &data, sizeof(data), 0);
            close(sockfd);
            sockfd = -1;
        }
        exit(0);
    }
};

int main()
{
    Client c;
    c.run();
}