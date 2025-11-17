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
#define MEDIUM_VALUE_LENGTH 50

// 用于存储配置信息的结构体
typedef struct
{
    char client_id[MEDIUM_VALUE_LENGTH];
    char topic[MAX_VALUE_LENGTH];
    char mac[MEDIUM_VALUE_LENGTH];
    char ip[MEDIUM_VALUE_LENGTH];
    char user[MAX_VALUE_LENGTH];
    char password[MAX_VALUE_LENGTH];
} Config;

Config config;

// 执行命令
char cmd_shutdown[800];

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
    size_t len = strlen(data);
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
    char key[MEDIUM_VALUE_LENGTH];
    char value[MEDIUM_VALUE_LENGTH];
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
                strncpy(cfg->client_id, value, MEDIUM_VALUE_LENGTH - 1);
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
                strncpy(cfg->mac, value, MEDIUM_VALUE_LENGTH - 1);
                clear_mark(cfg->mac);
            }
        }
        else if (strcmp(section, "openssh") == 0)
        {
            if (strcmp(key, "ip") == 0)
            {
                strncpy(cfg->ip, value, MEDIUM_VALUE_LENGTH - 1);
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

// 网络唤醒
int wol(const char *mac)
{
    if (mac == NULL || strlen(mac) != 17)
    { // MAC地址格式校验
        printf("Wol failed, because mac is null or incorrect format\n");
        return -1;
    }

    int ret = -1;
    ssize_t send_length = -1;
    unsigned char packet[102] = {0};
    struct sockaddr_in addr;
    int sockfd, i, option_value = 1;
    unsigned char mactohex[6] = {0};

    // 将MAC地址字符串转换为十六进制值
    sscanf(mac, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &mactohex[0], &mactohex[1], &mactohex[2],
           &mactohex[3], &mactohex[4], &mactohex[5]);

    // 构建魔术包
    for (i = 0; i < 6; i++)
    { // 6对“FF”前缀
        packet[i] = 0xFF;
    }
    for (i = 6; i < 102; i += 6)
    { // MAC地址重复16次
        memcpy(packet + i, mactohex, 6);
    }

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &option_value, sizeof(option_value));
    if (ret < 0)
    {
        printf("Set socket opt failed\n");
        close(sockfd);
        return ret;
    }

    // 设置目标地址为广播地址
    memset((void *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9);                            // 使用UDP端口9
    addr.sin_addr.s_addr = inet_addr("255.255.255.255"); // UDP广播地址

    // 设置权限,允许套接字发送广播数据包
    int broadcastPermission = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission)) < 0)
    {
        perror("setsockopt(SO_BROADCAST) failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    // 发送广播
    send_length = sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr));
    if (send_length < 0)
    {
        perror("sendto failed");
    }
    close(sockfd);
    return ret;
}

// 初始化命令
void init_cmd()
{
    // #局域网连接openssh服务器，进行关机操作 /h表示关机断电
    snprintf(cmd_shutdown, sizeof(cmd_shutdown), "sshpass -p %s ssh -A -g -o StrictHostKeyChecking=no %s@%s 'shutdown /h /t 10'", config.password, config.user, config.ip);
    // 打印命令
    printf("cmd_shutdown: %s\n", cmd_shutdown);
}

// 和巴法云的TCP连接
void connTCP()
{
    char substr[400]; // 确保这个缓冲区足够大
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
        exit(EXIT_FAILURE);
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
    snprintf(substr, sizeof(substr), "cmd=1&uid=%s&topic=%s\r\n", config.client_id, config.topic);
    send(tcp_client_socket, substr, strlen(substr), 0);
}

// 心跳函数
void *Ping()
{
    // 发送心跳
    const char *keeplive = "ping\r\n";
    while (1)
    { // 无限循环，每30秒发送一次心跳
        if (send(tcp_client_socket, keeplive, strlen(keeplive), 0) == -1)
        {
            perror("Send failed");
            break; // 如果发送失败，则退出循环
        }
        printf("Heartbeat sent\n");

        // 等待30秒
        sleep(30);
    }
    return NULL;
}

// 检查URL是否可达
char check_url(const char *url, int port, int timeout)
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
        perror("Socket creation failed\n");
        return 0; // 返回0表示失败
    }

    // 设置服务器地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, url, &addr.sin_addr) <= 0)
    {
        printf("Invalid address/ Address not supported\n");
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
void getMsgValue(const char *query, char *state)
{
    const char *key = "msg=";
    const char *start, *end;

    // 找到msg=的开始位置
    start = strstr(query, key);
    if (start)
    {
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
        strncpy(state, start, (size_t)(end - start));
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
    else
    {
        // 没有找到msg=
        printf("No msg found");
    }
}

// 处理数据
void process_data(char *recvData)
{
    size_t size = strlen(recvData);
    char data[size];
    memcpy(data, recvData, size);
    // 订阅主题正常返回
    if (strstr(data, "cmd=1&res=1"))
    {
        printf("Subscription topic successful.\n");
    }
    // 心跳正常返回
    else if (strstr(data, "cmd=0&res=1"))
    {
        printf("Heartbeat sent successful.\n");
    }
    // 接受到主题发布的数据
    else if (strstr(data, "cmd=2&uid="))
    {
        printf("Received topic publishing data.\n");
        // 解析数据
        char state[4] = {'\0'};
        char _on[] = "on";
        char _off[] = "off";
        getMsgValue(data, state);
        char pc_status = check_url(config.ip, 22, 3);
        if (strncmp(state, "on", 2) == 0)
        {
            printf("正在打开linux系统...\n");
            if (pc_status)
            {
                printf("当前目标linux在线,无须执行开机指令.\n");
            }
            else
            {
                // 网络唤醒
                if (wol(config.mac) >= 0)
                {
                    printf("linux系统开机指令发送成功!\n");
                }
                else
                {
                    printf("linux系统开机指令发送失败!\n");
                }
            }
        }
        else if (strncmp(state, "off", 3) == 0)
        {
            // 检查是否以 "off" 开头
            printf("正在关闭linux系统\n");
            if (pc_status)
            {
                printf("执行命令: %s\n", cmd_shutdown);
                system(cmd_shutdown);
            }
            else
            {
                printf("目标PC未在线或SSH服务器未开启\n");
            }
        }
        else
        {
            printf("The state does not start with 'on' or 'off'.\n");
        }
    }
    else
    {
        printf("Unexpected return!!!\n");
    }
}

int main()
{
    if (freopen("log.txt", "w", stdout) == NULL)
    {
        perror("Failed to open log.txt");
        return 1;
    }
    pthread_t ping_thread;
    parse_config("config.ini", &config);
    init_cmd();
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

    ssize_t len;
    char recvData[100];

    // 循环接收数据
    while (1)
    {
        // 初始化
        memset(recvData, 0, sizeof(recvData));
        len = recv(tcp_client_socket, recvData, sizeof(recvData) - 1, 0);
        if (len > 0)
        {
            recvData[len] = '\0'; // 确保字符串以空字符结尾
            printf("recv: %s", recvData);
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
