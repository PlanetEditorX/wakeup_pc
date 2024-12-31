#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
char cmd1[];
char cmd2[];
char cmd3[];

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
            }
            else if (strcmp(key, "topic") == 0)
            {
                strncpy(cfg->topic, value, MAX_VALUE_LENGTH - 1);
            }
        }
        else if (strcmp(section, "interface") == 0)
        {
            if (strcmp(key, "mac") == 0)
            {
                strncpy(cfg->mac, value, MAX_VALUE_LENGTH - 1);
            }
        }
        else if (strcmp(section, "openssh") == 0)
        {
            if (strcmp(key, "ip") == 0)
            {
                strncpy(cfg->ip, value, MAX_VALUE_LENGTH - 1);
            }
            else if (strcmp(key, "user") == 0)
            {
                strncpy(cfg->user, value, MAX_VALUE_LENGTH - 1);
            }
            else if (strcmp(key, "password") == 0)
            {
                strncpy(cfg->password, value, MAX_VALUE_LENGTH - 1);
            }
        }
    }

    fclose(file);
}

// 初始化命令
void init_cmd(Config config)
{
    char command[256];
    // 去除MAC地址两端的双引号（如果有）
    char *mac = config.mac;
    if (mac[0] == '\"')
    {
        memmove(mac, mac + 1, strlen(mac) - 1);
    }
    int len = strlen(mac);
    if (mac[len - 1] == '\"')
    {
        mac[len - 1] = '\0';
    }

    // 构建wakeonlan命令
    snprintf(command, sizeof(command), "wakeonlan %s", mac);

    // 打印命令
    printf("Command: %s\n", command);
}



int main()
{
    parse_config("config.ini", &config);
    init_cmd(config);

    // 打印配置信息以验证
    printf("client_id: %s\n", config.client_id);
    printf("topic: %s\n", config.topic);
    printf("mac: %s\n", config.mac);
    printf("ip: %s\n", config.ip);
    printf("user: %s\n", config.user);
    printf("password: %s\n", config.password);

    return 0;
}