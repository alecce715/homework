#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1" // 替换为服务器IP

// 计算ICMP校验和（同服务器端）
uint16_t calc_checksum(void *buffer, int len) {
    uint32_t sum = 0;
    uint16_t *buf = (uint16_t *)buffer;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) sum += *(uint8_t *)buf;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

// 获取当前时间（从UTC午夜开始的毫秒数）
uint32_t get_current_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm = gmtime(&tv.tv_sec);
    uint32_t ms = (tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec) * 1000;
    ms += tv.tv_usec / 1000;
    return ms;
}

int main() {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // 构造ICMP时间戳请求
    struct icmphdr request_icmp;
    memset(&request_icmp, 0, sizeof(request_icmp));
    request_icmp.type = ICMP_TIMESTAMP;
    request_icmp.code = 0;
    request_icmp.un.echo.id = htons(getpid()); // 用进程ID作为标识符
    request_icmp.un.echo.sequence = htons(1);  // 序列号从1开始

    uint32_t originate_ts = get_current_ms();
    char *request_data = (char *)&request_icmp + 8;
    *(uint32_t *)request_data = htonl(originate_ts);
    *(uint32_t *)(request_data + 4) = 0; // 接收时间戳留空
    *(uint32_t *)(request_data + 8) = 0; // 发送时间戳留空

    // 计算校验和
    request_icmp.checksum = 0;
    request_icmp.checksum = calc_checksum(&request_icmp, sizeof(request_icmp));

    // 发送请求
    if (sendto(sockfd, &request_icmp, sizeof(request_icmp), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Sent ICMP Timestamp Request to " << SERVER_IP << std::endl;

    // 接收应答
    char buffer[BUFFER_SIZE];
    struct sockaddr_in reply_addr;
    socklen_t addr_len = sizeof(reply_addr);
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&reply_addr, &addr_len);
    if (recv_len < 0) {
        perror("recvfrom");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct icmphdr *reply_icmp = (struct icmphdr *)buffer;
    if (reply_icmp->type != ICMP_TIMESTAMPREPLY || reply_icmp->code != 0) {
        std::cerr << "Invalid ICMP reply type: " << (int)reply_icmp->type << std::endl;
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 解析应答时间戳
    uint32_t reply_originate = ntohl(*(uint32_t *)(buffer + 8));
    uint32_t reply_receive = ntohl(*(uint32_t *)(buffer + 12));
    uint32_t reply_transmit = ntohl(*(uint32_t *)(buffer + 16));
    uint32_t current_ts = get_current_ms();

    std::cout << "\nICMP Time Reply from " << inet_ntoa(reply_addr.sin_addr) << std::endl;
    std::cout << "Originate Timestamp: " << reply_originate << " ms" << std::endl;
    std::cout << "Receive Timestamp: " << reply_receive << " ms" << std::endl;
    std::cout << "Transmit Timestamp: " << reply_transmit << " ms" << std::endl;
    std::cout << "Round-Trip Time (RTT): " << (current_ts - reply_originate) << " ms" << std::endl;

    close(sockfd);
    return 0;
}
