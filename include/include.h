/*
 * @Author: Yinwhe
 * @Date: 2021-12-19 10:39:53
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-12-19 19:34:13
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */

#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <mutex>
#include <cstring>
#include <vector>
#include <pthread.h>

enum msgType {CONNECT = 1, DISCONNECT, GET_TIME, GET_NAME, GET_CLIENT_LIST, SEND_MSG, REPOST};

const int MAX_SIZE = 256;

struct Message {
    long type;
    char data[MAX_SIZE - 2];
};