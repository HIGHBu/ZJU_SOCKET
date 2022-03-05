/*
 * @Author: GCX
 * @Date: 2021-12-19 10:39:35
 * @LastEditors: GCX
 * @LastEditTime: 2021-12-19 19:37:52
 * @Description: Server part
 * @Copyright: Copyright (c) 2021
 */

#include "include.h"
using namespace std;

#define SERVER_PORT 3302
#define MAX_QUEUE_LENGTH 20

typedef struct sock_addr_port
{                //句柄、地址和端口号
    int sock;    //句柄
    string addr; //地址
    int port;    //端口号
} sock_addr_port;
vector<sock_addr_port> clients;
mutex mtx;

static void *thread_handle(void *cfd)
{
    int conn_fd = *(int *)cfd; //连接到的句柄号
    char str_hello[] = "hello";
    send(conn_fd, str_hello, sizeof(str_hello), 0); //发送hello消息
    cout << "say hello" << endl;
    int flag = -1;
    Message mes_rec;
    while (flag == -1)
    {
        if (recv(conn_fd, &mes_rec, sizeof(mes_rec), 0) < 0)
        {
            cout << "[Error] recv failed, error no is " << errno << endl;
            continue;
        }
        mtx.lock();
        switch (mes_rec.type)
        {
        case DISCONNECT:
        {
            for (int i = 0; i < clients.size(); i++)
            {
                if (clients[i].sock == conn_fd)
                {
                    std::vector<sock_addr_port>::iterator it = clients.begin() + i;
                    clients.erase(it);
                    break;
                }
            }
            cout << "Disconnect Client" + conn_fd << endl;
            close(conn_fd);
            flag = 1;
            break;
        }
        case GET_TIME:
        {
            time_t t;
            time(&t);
            Message mes;
            mes.type = GET_TIME;
            sprintf(mes.data, "%ld", t);
            if (send(conn_fd, &mes, sizeof(mes), 0) < 0)
            {
                cout << "[Error] send failed, error no is " << errno << endl;
            }
            cout << "Client " << conn_fd << "get time" << endl;
            printf("%s\n",mes.data);
            break;
        }
        case GET_NAME:
        {
            Message mes;
            mes.type = GET_NAME;
            gethostname(mes.data, sizeof(mes.data));
            if (send(conn_fd, &mes, sizeof(mes), 0) < 0)
            {
                cout << "[Error] send failed, error no is " << errno << endl;
            }
            cout << "Client " << conn_fd << " get server name" << endl;
            printf("%s\n",mes.data);
            break;
        }
        case GET_CLIENT_LIST:
        {
            Message mes;
            cout << "Client " << conn_fd << " get client lists" << endl;
            mes.type = GET_CLIENT_LIST;
            for (int i = 0; i < clients.size(); i++)
            {
                int clnt_sock = clients[i].sock;
                int clnt_port = clients[i].port;
                string clnt_addr = clients[i].addr;
                string c = "handler " + to_string(clnt_sock) + " ip:host " + clnt_addr + ":" + to_string(clnt_port);
                char tmp[256]={};
                strcpy(tmp,c.c_str());
                memcpy(mes.data + strlen(mes.data), tmp, c.length());
                // strcat(mes.data,tmp);
                cout << c << endl;
            }
            if (send(conn_fd, &mes, sizeof(mes), 0) < 0)
            {
                cout << "[Error] send failed, error no is " << errno << endl;
            }
            break;
        }
        case SEND_MSG:
            // ip:端口:data
            {
                string data = string(mes_rec.data);
                string ip = data.substr(0, data.find(":"));
                data = data.substr(data.find(":") + 1);
                int port = stoi(data.substr(0, data.find(":")));
                data = data.substr(data.find(":") + 1);
                cout << "Client" << conn_fd << " send message to: " << ip << ":" << port << endl;
                int target = -1;
                for (int i = 0; i < clients.size(); i++)
                {
                    if (clients[i].addr == ip && clients[i].port == port)
                    {
                        target = i;
                        break;
                    }
                }
                Message mes;
                mes.type = SEND_MSG;
                if (target == -1)
                { //没有目标
                    sprintf(mes.data, "no such client");
                }
                else
                {
                    sprintf(mes.data, "forward success");
                    Message mes_fwd;
                    mes_fwd.type = REPOST;
                    strcpy(mes_fwd.data, data.c_str());
                    if (send(clients[target].sock, &mes_fwd, sizeof(mes_fwd), 0) < 0)
                    {
                        cout << "[Error] send failed, error no is " << errno << endl;
                    }
                }
                if (send(conn_fd, &mes, sizeof(mes), 0) < 0)
                {
                    cout << "[Error] send failed, error no is " << errno << endl;
                }
                break;
            }
        }
        memset(&mes_rec, 0, sizeof(mes_rec));
        mtx.unlock();
    }
    return NULL;
}
int main()
{
    cout << "start" << endl;
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERVER_PORT);
    bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(serv_sock, MAX_QUEUE_LENGTH);
    cout << "listening" << endl;
    while (1)
    {
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        cout<<"waiting for connect"<<endl;
        int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        // clnt_sock用于跟客户端通信

        sock_addr_port clnt;
        clnt.sock = clnt_sock;
        clnt.addr = inet_ntoa(clnt_addr.sin_addr);
        clnt.port = ntohs(clnt_addr.sin_port);
        clients.push_back(clnt);
        //句柄，地址，端口号
        cout << "get connect from " + clnt.addr + ": " + to_string(clnt.port) + ",its handle is " + to_string(clnt_sock) << endl;

        pthread_t conn_thread;
        pthread_create(&conn_thread, NULL, thread_handle, &clnt_sock);
    }
    return 0;
}