#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

// 服务端的流程
//     1.创建socket套接字
//     2.连接服务器
//     3.开始通讯
//     4.关闭连接
//     5.开始通讯
//     6.关闭连接


int main() {

    // 1.创建socket套接字
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Socket creation failed!!!");
        exit(EXIT_FAILURE);
    }
    // 2.连接服务器
    struct sockaddr_in targe;
    targe.sin_family = AF_INET;
    targe.sin_port = htons(8344);
    targe.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(client_socket, (struct sockaddr *)&targe, sizeof(targe)) == -1)
    {
        printf("Connect server failed!!!\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    // 3.开始通讯
    while (1)
    {
        char sbuffer[1024] = { 0 };
        printf("Please enter：");
        scanf("Client send: %s", sbuffer);

        send(client_socket, sbuffer, strlen(sbuffer), 0);

        char rbuffer[1024] = { 0 };
        int ret = recv(client_socket, rbuffer, 1024, 0);
        if (ret <= 0 )
        {
            printf("Connect break!!!\n");
            break;
        }
        printf("Feedback: %s\n", rbuffer);
    }

    // 4.关闭连接
    close(client_socket);
    return 0;
}