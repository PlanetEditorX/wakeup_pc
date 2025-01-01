#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

int listen_socket;

// 服务端的流程
//     1.创建s0cket套接字
//     2.给这个socket绑定一个端口号
//     3.给这个socket开后监听属性
//     4.等待客户端连接
//     5.开始通讯
//     6.关闭连接
int main()
{
    // 1.创建socket套接字
    // socket (
    //     int __domain, // 协议地址簇 IPV4/IPV6 AF_INET/AF_INET6
    //     int __type, // 类型 流式协议 帧式协议 SOCK_STREAM/SOCK_DGRAM
    //     int __protocol // 保护协议 0
    // )
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1)
    {
        perror("Socket creation failed!!!");
        exit(EXIT_FAILURE);
    }

    // 2.给这个socket:绑定一个端口号
    // struct sockaddr_in
    // {
    //     ADDRESS FAMILY sin_family; // 协议地址簇
    //     in_port_t sin_port;      // 端口号
    //     struct in_addr sin_addr; // IP地址
    //     unsigned char sin_zero[sizeof(struct sockaddr) // 保留字节
    // };
    struct sockaddr_in local = { 0 };
    local.sin_family = AF_INET;
    local.sin_port = htons(8344);
    // 服务端 选项 网卡127.0.0.1(本地环回)只接受哪个网卡的数据，一般写全0地址表示全部都接受
    // local.sin_addr.s_addr = inet_addr("0.0.0.0"); // 字符串IP地址转换成整数IP
    local.sin_addr.s_addr = htonl(INADDR_ANY); // 等效于全0地址
    // 绑定
    // int bind(
    //     SOCKET s,
    //     const struct sockaddr * name,
    //     int namelen
    // )
    // struct sockaddr
    // {
    //     __SOCKADDR_COMMON(sa_); /* Common data: address family and length.  */
    //     char sa_data[14];       /* Address data.  */
    // };
    if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local)) == -1)
    {
        perror("Socket bind failed!!!");
        exit(EXIT_FAILURE);
    }

    // 3.给这个socket开启监听属性
    // int listen (
    //      SOCKET s,
    //      int backlog
    // )
    if (listen(listen_socket, 10) == -1)
    {
        perror("start listen failed!!!");
        exit(EXIT_FAILURE);
    }

    // 4.等待客户端连接
    // 返回的客户端socket才是跟客户端可以通讯的一个socket
    // 阻塞函数，等到有客户端连接进来就接受连接，然后返回，否则就阻塞
    // int accept(
    //     int __fd,                        // 监听socket
    //     __SOCKADDR_ARG __addr,           // 客户端的iIP地址和端口号
    // 	socklen_t *__restrict __addr_len    // 结构的大小
    // )
    while (1)
    {
        int client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == -1)
        {
            continue;
        }
        // 多个客户端就要创建线程
        while (1) {
            // 5. 开始通讯(B/S)
            char buffer[1024] = {0};
            // recv (
            //     int __fd,    // 客户端socket
            //     void *__buf, // 接受的数据存到哪里
            //     size_t __n,  // 接受的长度
            //     int __flags  // 0
            // )
            // 接受客户端数据
            int ret = recv(client_socket, buffer, 1024, 0);
            printf("%s\n", buffer);
            // 向客户端发送数据
            send(client_socket, buffer, strlen(buffer), 0);
            if (ret <= 0)
            {
                printf("Connect break!!!\n");
                break;
            }
        }

        // 6.关闭连接：服务端不应该断开连接
        close(client_socket);
    }



    return 0;
}