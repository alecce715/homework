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

// 计算ICMP校验和
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
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    std::cout << "ICMP Time Server running..." << std::endl;

    while (true) {
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("recvfrom");
            continue;
        }

        struct icmphdr *icmp_hdr = (struct icmphdr *)buffer;
        // 仅处理ICMP时间戳请求（类型13）
        if (icmp_hdr->type != ICMP_TIMESTAMP || icmp_hdr->code != 0) {
            continue;
        }

        // 提取请求参数
        uint16_t id = ntohs(icmp_hdr->un.echo.id);
        uint16_t seq = ntohs(icmp_hdr->un.echo.sequence);
        uint32_t originate_ts = ntohl(*(uint32_t *)(buffer + 8));

        // 构造应答报文
        struct icmphdr reply_icmp;
        memset(&reply_icmp, 0, sizeof(reply_icmp));
        reply_icmp.type = ICMP_TIMESTAMPREPLY;
        reply_icmp.code = 0;
        reply_icmp.un.echo.id = htons(id);
        reply_icmp.un.echo.sequence = htons(seq);
        uint32_t receive_ts = get_current_ms();
        uint32_t transmit_ts = receive_ts; // 简化处理：接收即发送

        // 填充时间戳字段
        char *reply_data = (char *)&reply_icmp + 8;
        *(uint32_t *)reply_data = htonl(originate_ts);
        *(uint32_t *)(reply_data + 4) = htonl(receive_ts);
        *(uint32_t *)(reply_data + 8) = htonl(transmit_ts);

        // 计算校验和
        reply_icmp.checksum = 0;
        reply_icmp.checksum = calc_checksum(&reply_icmp, sizeof(reply_icmp));

        // 发送应答
        if (sendto(sockfd, &reply_icmp, sizeof(reply_icmp), 0,
                   (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("sendto");
        } else {
            std::cout << "Replied to " << inet_ntoa(client_addr.sin_addr)
                      << " (ID: " << id << ", Seq: " << seq << ")" << std::endl;
        }
    }

    close(sockfd);
    return 0;
}
