#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_LINE_LENGTH 1024
#define MAX_SECTION_LENGTH 256
#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 256

// 用于存储配置信息的结构体
typedef struct
{
    char client_id[MAX_VALUE_LENGTH];
    char topic[MAX_VALUE_LENGTH];
    char mac[MAX_VALUE_LENGTH];
    char ip[MAX_VALUE_LENGTH];
    char user[MAX_VALUE_LENGTH];
    char password[MAX_VALUE_LENGTH];
} Config;

Config config;

// 执行命令
char cmd1[256];
char cmd2[256];
char cmd3[256];

// 套接字持续化连接
int tcp_client_socket;

// 清空配置结构体
void clear_config(Config *cfg)
{
    memset(cfg->client_id, 0, MAX_VALUE_LENGTH);
    memset(cfg->topic, 0, MAX_VALUE_LENGTH);
    memset(cfg->mac, 0, MAX_VALUE_LENGTH);
    memset(cfg->ip, 0, MAX_VALUE_LENGTH);
    memset(cfg->user, 0, MAX_VALUE_LENGTH);
    memset(cfg->password, 0, MAX_VALUE_LENGTH);
}

// 去除引号
void clear_mark(char *data)
{
    if (data[0] == '\"')
    {
        memmove(data, data + 1, strlen(data) - 1);
    }
    int len = strlen(data);
    while (data[len - 1] == '\"')
    {
        data[len - 1] = '\0';
        len = strlen(data);
    }
}

// 解析配置文件
void parse_config(const char *filename, Config *cfg)
{

    FILE *file;
    char line[MAX_LINE_LENGTH];
    char section[MAX_SECTION_LENGTH];
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int reading_section = 0;

    clear_config(cfg);
    file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("打开文件失败");
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        // 忽略空行和注释行
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#')
        {
            continue;
        }

        // 检查是否是节
        if (line[0] == '[')
        {
            sscanf(line, "[%[^]]]", section);
            reading_section = 1; // 标记正在读取节
            continue;
        }

        // 如果不是节，并且正在读取节，则停止读取
        if (reading_section == 0)
        {
            continue;
        }

        // 提取键和值
        sscanf(line, "%s = %s", key, value);
        // 根据节和键更新配置信息
        if (strcmp(section, "bafa") == 0)
        {
            if (strcmp(key, "client_id") == 0)
            {
                strncpy(cfg->client_id, value, MAX_VALUE_LENGTH - 1);
                clear_mark(cfg->client_id);
            }
            else if (strcmp(key, "topic") == 0)
            {
                strncpy(cfg->topic, value, MAX_VALUE_LENGTH - 1);
                clear_mark(cfg->topic);
            }
        }
        else if (strcmp(section, "interface") == 0)
        {
            if (strcmp(key, "mac") == 0)
            {
                strncpy(cfg->mac, value, MAX_VALUE_LENGTH - 1);
                clear_mark(cfg->mac);
            }
        }
        else if (strcmp(section, "openssh") == 0)
        {
            if (strcmp(key, "ip") == 0)
            {
                strncpy(cfg->ip, value, MAX_VALUE_LENGTH - 1);
                clear_mark(cfg->ip);
            }
            else if (strcmp(key, "user") == 0)
            {
                strncpy(cfg->user, value, MAX_VALUE_LENGTH - 1);
                clear_mark(cfg->user);
            }
            else if (strcmp(key, "password") == 0)
            {
                strncpy(cfg->password, value, MAX_VALUE_LENGTH - 1);
                clear_mark(cfg->password);
            }
        }
    }
    fclose(file);
}


// 初始化命令
void init_cmd(Config *config)
{
    // 网络唤醒
    snprintf(cmd1, sizeof(cmd1), "wakeonlan %s", config->mac);
    // 发送关闭电脑off给巴法云，更新状态
    snprintf(cmd2, sizeof(cmd2), "/usr/bin/curl -s \"https://api.bemfa.com/api/device/v1/data/3/push/get/?uid=%s&topic=%s&msg=off\"", config->client_id, config->topic);
    // #局域网连接openssh服务器，进行关机操作
    snprintf(cmd3, sizeof(cmd3), "sshpass -p %s ssh -A -g -o StrictHostKeyChecking=no %s@%s shutdown - s - t 10", config->password, config->user, config->ip);
    // 打印命令
    printf("cmd1: %s\n", cmd1);
    printf("cmd2: %s\n", cmd2);
    printf("cmd3: %s\n", cmd3);
}


// 和巴法云的TCP连接
void connTCP()
{
    struct sockaddr_in server_addr;
    char substr[256]; // 确保这个缓冲区足够大

    // 创建socket
    tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    char *ipaddr;
    // IP 和端口
    char *hostname = "bemfa.com";
    struct hostent *hptr;

    if ((hptr = gethostbyname(hostname)) == NULL)
    {
        printf("gethotbyname error\n");
        exit(EXIT_FAILURE);
    }
    char addressContent[32];
    char *address;

    address = inet_ntop(2, hptr, addressContent, sizeof(addressContent));
    // if (inet_pton(2, address, &server_addr.sin_addr) <= 0)
    // {
    //     perror("inet_pton failed");
    //     // 处理错误
    // }
    printf(address);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8344);                  // 端口号需要转换为网络字节序
    serv_addr.sin_addr.s_addr = address;               // 将点分十进制 IP 地址转换为二进制形式

    // server_addr.sin_addr = address;

    // 连接服务器
    if (connect(tcp_client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    printf('connect......');

    // 发送订阅指令
    sprintf(substr, "cmd=1&uid=%s&topic=%s\r\n", config.client_id, config.topic);
    send(tcp_client_socket, substr, strlen(substr), 0);

    // 关闭socket
    close(tcp_client_socket);
}

// 心跳函数
void *Ping(void *arg)
{
    const char *keeplive = "ping\r\n";
    struct timespec timer_wait = {.tv_sec = 30}; // 30秒

    while (1)
    {
        send(tcp_client_socket, keeplive, strlen(keeplive), 0);

        // 休眠30秒
        nanosleep(&timer_wait, NULL);
    }

    return NULL;
}

// 检查URL是否可达
int check_url(const char *url, int port, int timeout)
{
    int sock;
    struct sockaddr_in addr;
    struct timeval tv;
    fd_set writefds;

    // 设置socket超时
    if (timeout > 0)
    {
        sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    }
    else
    {
        // 如果不设置超时，就使用阻塞socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (sock < 0)
    {
        perror("Socket creation failed");
        return 0; // 返回0表示失败
    }

    // 设置服务器地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, url, &addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(sock);
        return 0; // 返回0表示失败
    }

    // 尝试连接
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    // 设置超时
    if (timeout > 0)
    {
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);
        tv.tv_sec = timeout; // 等待3秒
        tv.tv_usec = 0;

        // 检查是否连接成功
        if (select(sock + 1, NULL, &writefds, NULL, &tv) > 0)
        {
            if (FD_ISSET(sock, &writefds))
            {
                // 连接成功
                close(sock);
                return 1; // 返回1表示成功
            }
        }
    }

    // 连接失败
    close(sock);
    return 0; // 返回0表示失败
}

void process_data(char *data)
{
    char *sw;
    // 解析数据
    sw = strstr(data, "sw=") + 3;
    if (sw && strcmp(sw, "on") == 0)
    {
        printf("正在打开电脑\n");
        system(cmd1);
    }
    else
    {
        printf("正在关闭电脑\n");
        printf("执行命令: %s\n", cmd2);
        system(cmd2);
        if (check_url("ssh_ip", 22, 10))
        { // 假设ssh_ip是SSH服务器的IP
            printf("执行命令: %s\n", cmd3);
            system(cmd3);
        }
        else
        {
            printf("目标PC未在线或SSH服务器未开启\n");
        }
    }
}

int main()
{
    pthread_t ping_thread;
    parse_config("config.ini", &config);
    init_cmd(&config);

    connTCP();

    // 创建心跳线程
    if (pthread_create(&ping_thread, NULL, Ping, NULL) != 0)
    {
        perror("Failed to create ping thread");
        exit(EXIT_FAILURE);
    }

    // 让心跳线程运行在后台
    pthread_detach(ping_thread);

    char recvData[1024];
    int len;

    // 循环接收数据
    while (1)
    {
        len = recv(tcp_client_socket, recvData, sizeof(recvData) - 1, 0);
        if (len > 0)
        {
            recvData[len] = '\0'; // 确保字符串以空字符结尾
            printf("recv: %s\n", recvData);
            process_data(recvData);
        }
        else if (len == 0)
        {
            printf("conn err\n");
            connTCP();
        }
        else
        {
            perror("recv failed");
            break;
        }
        sleep(2); // 睡眠2秒，避免过快重试
    }

    return 0;
}