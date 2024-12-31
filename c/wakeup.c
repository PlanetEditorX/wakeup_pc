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
int tcp_client_socket = -1;

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
    char substr[256]; // 确保这个缓冲区足够大
    // 1.创建socket套接字
    tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_socket == -1)
    {
        perror("Socket creation failed!!!");
        exit(EXIT_FAILURE);
    }
    // 2.连接服务器
    struct sockaddr_in targe;
    targe.sin_family = AF_INET;
    targe.sin_port = htons(8344);
    // IP 和端口
    char *hostname = "bemfa.com";
    struct hostent *hptr;

    if ((hptr = gethostbyname(hostname)) == NULL)
    {
        printf("Gethotbyname error\n");
        exit(EXIT_FAILURE);
    }
    // 获取第一个 IP 地址
    struct in_addr **addr_list = (struct in_addr **)hptr->h_addr_list;
    struct in_addr *first_addr = addr_list[0];
    if (first_addr == NULL)
    {
        fprintf(stderr, "No IP address found for host: %s\n", hostname);
        return 1;
    }
    // 将第一个 IP 地址转换为字符串形式
    char ip_str[INET_ADDRSTRLEN];
    // inet_ntop 函数用于将一个二进制的网络地址（IPv4 或 IPv6）转换为一个以 null 结尾的字符串。这个函数支持 IPv4 和 IPv6
    inet_ntop(AF_INET, &(first_addr->s_addr), ip_str, INET_ADDRSTRLEN);

    printf("The IP address of %s is: %s\n", hostname, ip_str);
    // inet_addr 函数用于将一个点分十进制的 IPv4 地址字符串转换为网络字节序的二进制值,如果转换成功，返回网络字节序的二进制值
    targe.sin_addr.s_addr = inet_addr(ip_str);
    if (connect(tcp_client_socket, (struct sockaddr *)&targe, sizeof(targe)) == -1)
    {
        printf("Connect server failed!!!\n");
        close(tcp_client_socket);
        exit(EXIT_FAILURE);
    }
    // 发送订阅指令
    sprintf(substr, "cmd=1&uid=%s&topic=%s\r\n", config.client_id, config.topic);
    send(tcp_client_socket, substr, strlen(substr), 0);

    // 关闭socket
    // close(tcp_client_socket);
}

// 心跳函数
void *Ping(void *arg)
{
    // 发送心跳
    const char *keeplive = "ping\r\n";
    // send(tcp_client_socket, keeplive, strlen(keeplive), 0);
    while (1)
    { // 无限循环，每30秒发送一次心跳
        if (send(tcp_client_socket, keeplive, strlen(keeplive), 0) == -1)
        {
            perror("send failed");
            break; // 如果发送失败，则退出循环
        }
        printf("Heartbeat sent\n");

        // 等待30秒
        sleep(30);
    }
    // 开启定时，30秒发送一次心跳
    // pthread_t timer_thread;
    // int result = pthread_create(&timer_thread, NULL, Ping, NULL);
    // if (result != 0)
    // {
    //     perror("pthread_create");
    //     exit(EXIT_FAILURE);
    // }
    // pthread_detach(timer_thread); // 分离线程，自动回收资源
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

// 函数用于解析查询字符串并返回msg的值
const char *getMsgValue(const char *query, char *state)
{
    const char *key = "msg=";
    const char *start, *end;

    // 找到msg=的开始位置
    start = strstr(query, key);
    if (start == NULL)
    {
        return NULL; // 没有找到msg=
    }

    // 跳过key的长度，指向msg值的开始位置
    start += strlen(key);

    // 找到msg值的结束位置（即&符号的位置）
    end = strchr(start, '&');
    if (end == NULL)
    {
        // 没有找到&，说明msg是最后一个参数
        end = start + strlen(start);
    }

    // 复制msg值到一个新的字符串中
    state[end - start + 1];
    strncpy(state, start, end - start);
    state[end - start] = '\0'; // 确保字符串以'\0'结尾
    while (*state)
    { // 遍历字符串直到遇到'\0'
        if (*state == '\r' || *state == '\n')
        {
            *state = '\0'; // 替换'\r'或'\n'为'\0'
        }
        state++; // 移动到下一个字符
    }
}

void process_data(char *recvData)
{
    // 解析数据
    char state[3] = { '\0' };
    char _on[] = "on";
    char _off[] = "off";
    getMsgValue(recvData, &state);
    if (state[0] != '\0')
    {
        if (strcmp(state, _on) == 0)
        {
            printf("正在打开电脑\n");
            system(cmd1);
        }
        else if (strcmp(state, _off) == 0)
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

}

int main()
{
    pthread_t ping_thread;
    parse_config("config.ini", &config);
    init_cmd(&config);
    // 和巴法云进行TCP连接
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