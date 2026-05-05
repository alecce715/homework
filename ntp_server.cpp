#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>

#define NTP_PORT 123
#define NTP_PACKET_SIZE 48

// NTP时间戳结构体（64位：32位秒 + 32位微秒）
struct ntp_timestamp {
    uint32_t seconds;
    uint32_t fraction;
};

// 将时间字符串转换为NTP时间戳
ntp_timestamp parse_time(const char* time_str) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    
    // 解析时间字符串 "2019-05-01 10:41:00"
    strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm);
    
    // 转换为time_t（自1970-01-01以来的秒数）
    time_t unix_time = mktime(&tm);
    
    // NTP时间戳起点是1900-01-01，Unix时间戳起点是1970-01-01
    // 相差70年 = 2208988800秒
    const uint32_t NTP_OFFSET = 2208988800UL;
    
    ntp_timestamp ts;
    ts.seconds = static_cast<uint32_t>(unix_time) + NTP_OFFSET;
    ts.fraction = 0;  // 简化：微秒部分为0
    
    return ts;
}

// 创建NTP响应包
void create_ntp_response(char* packet, ntp_timestamp transmit_time) {
    memset(packet, 0, NTP_PACKET_SIZE);
    
    // LI（闰秒指示器）= 0，VN（版本号）= 4，Mode（模式）= 4（服务器）
    packet[0] = 0x24;  // 00100100
    
    // Stratum（层级）= 1（主参考源）
    packet[1] = 1;
    
    // Poll（轮询间隔）= 4（2^4 = 16秒）
    packet[2] = 4;
    
    // Precision（精度）= -6（2^-6 = 1/64秒）
    packet[3] = 0xFA;  // -6的补码
    
    // Root Delay（根延迟）= 0
    uint32_t root_delay = 0;
    memcpy(packet + 4, &root_delay, 4);
    
    // Root Dispersion（根离散）= 0
    uint32_t root_dispersion = 0;
    memcpy(packet + 8, &root_dispersion, 4);
    
    // Reference Identifier（参考标识符）= "LOCL"（本地时钟）
    packet[12] = 'L';
    packet[13] = 'O';
    packet[14] = 'C';
    packet[15] = 'L';
    
    // Reference Timestamp（参考时间戳）= 当前时间
    uint32_t ref_sec = htonl(transmit_time.seconds);
    uint32_t ref_frac = htonl(transmit_time.fraction);
    memcpy(packet + 16, &ref_sec, 4);
    memcpy(packet + 20, &ref_frac, 4);
    
    // Originate Timestamp（原始时间戳）= 从客户端请求中获取
    // 这里简化为0，实际应从请求包中提取
    memset(packet + 24, 0, 8);
    
    // Receive Timestamp（接收时间戳）= 当前时间
    uint32_t recv_sec = htonl(transmit_time.seconds);
    uint32_t recv_frac = htonl(transmit_time.fraction);
    memcpy(packet + 32, &recv_sec, 4);
    memcpy(packet + 36, &recv_frac, 4);
    
    // Transmit Timestamp（传输时间戳）= 当前时间
    uint32_t xmit_sec = htonl(transmit_time.seconds);
    uint32_t xmit_frac = htonl(transmit_time.fraction);
    memcpy(packet + 40, &xmit_sec, 4);
    memcpy(packet + 44, &xmit_frac, 4);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "用法: " << argv[0] << " \"YYYY-MM-DD HH:MM:SS\"" << std::endl;
        std::cerr << "示例: " << argv[0] << " \"2019-05-01 10:41:00\"" << std::endl;
        return 1;
    }
    
    // 解析时间字符串
    ntp_timestamp target_time = parse_time(argv[1]);
    
    // 创建UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(NTP_PORT);
    
    // 绑定端口
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }
    
    std::cout << "NTP服务器运行中，设置时间为: " << argv[1] << std::endl;
    std::cout << "等待Windows客户端请求..." << std::endl;
    
    char buffer[NTP_PACKET_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (true) {
        // 接收NTP请求
        ssize_t recv_len = recvfrom(sockfd, buffer, NTP_PACKET_SIZE, 0,
                                   (struct sockaddr*)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("recvfrom");
            continue;
        }
        
        std::cout << "收到来自 " << inet_ntoa(client_addr.sin_addr) << " 的请求" << std::endl;
        
        // 创建NTP响应
        char response[NTP_PACKET_SIZE];
        create_ntp_response(response, target_time);
        
        // 发送响应
        if (sendto(sockfd, response, NTP_PACKET_SIZE, 0,
                  (struct sockaddr*)&client_addr, addr_len) < 0) {
            perror("sendto");
        } else {
            std::cout << "已响应时间请求" << std::endl;
        }
    }
    
    close(sockfd);
    return 0;
}
