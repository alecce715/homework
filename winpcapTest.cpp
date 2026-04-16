#include <iostream>
#include <string>
#include <ctime>
#include <fstream>
#include <map>
#include <chrono>
#include <thread>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <winsock2.h>
#include <windows.h>
#include <pcap.h>
#include <iphlpapi.h>

#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// 以太网帧头结构
typedef struct eth_header {
    u_char dest_mac[6];  // 目的MAC地址
    u_char src_mac[6];   // 源MAC地址
    u_short ether_type;  // 网络层协议类型
} eth_header;

// IPv4头部结构
typedef struct ip_header {
    u_char  ver_ihl;        // 版本(4位) + 头部长度(4位)
    u_char  tos;            // 服务类型
    u_short total_len;      // 总长度
    u_short ident;          // 标识
    u_short flags_fo;       // 标志(3位) + 片偏移(13位)
    u_char  ttl;            // 生存时间
    u_char  protocol;       // 协议类型
    u_short checksum;       // 头部校验和
    u_int   src_addr;       // 源IP地址
    u_int   dst_addr;       // 目的IP地址
} ip_header;

// 全局变量
std::atomic<bool> running(true);  // 控制程序运行的标志
std::ofstream log_file;           // 日志文件
std::mutex stats_mutex;           // 保护统计数据的互斥锁

// 统计数据结构
struct Stats {
    std::map<std::string, uint64_t> src_mac_stats;  // 源MAC统计
    std::map<std::string, uint64_t> src_ip_stats;   // 源IP统计
    std::map<std::string, uint64_t> dst_mac_stats;  // 目的MAC统计
    std::map<std::string, uint64_t> dst_ip_stats;   // 目的IP统计
} stats;

// 将MAC地址转换为字符串格式
std::string macToString(const u_char* mac) {
    char buffer[18];
    sprintf_s(buffer, sizeof(buffer), "%02X-%02X-%02X-%02X-%02X-%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buffer);
}

// 将IP地址转换为字符串格式
std::string ipToString(u_int ip) {
    struct in_addr addr;
    addr.s_addr = ip;
    return std::string(inet_ntoa(addr));
}

// 获取当前时间字符串
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

// 数据包处理回调函数
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* packet) {
    eth_header* eth = (eth_header*)packet;
    
    // 获取MAC地址
    std::string src_mac = macToString(eth->src_mac);
    std::string dst_mac = macToString(eth->dest_mac);
    
    // 初始化IP地址
    std::string src_ip = "0.0.0.0";
    std::string dst_ip = "0.0.0.0";
    
    // 检查是否为IP包
    if (ntohs(eth->ether_type) == 0x0800) {  // IPv4
        ip_header* iph = (ip_header*)(packet + sizeof(eth_header));
        
        // 获取IP地址
        src_ip = ipToString(iph->src_addr);
        dst_ip = ipToString(iph->dst_addr);
    }
    
    // 获取当前时间和帧长度
    std::string timestamp = getCurrentTime();
    int frame_len = header->len;
    
    // 加锁保护共享资源
    std::lock_guard<std::mutex> lock(stats_mutex);
    
    // 更新统计信息
    stats.src_mac_stats[src_mac] += frame_len;
    stats.dst_mac_stats[dst_mac] += frame_len;
    stats.src_ip_stats[src_ip] += frame_len;
    stats.dst_ip_stats[dst_ip] += frame_len;
    
    // 写入日志文件
    if (log_file.is_open()) {
        log_file << timestamp << ","
                 << src_mac << ","
                 << src_ip << ","
                 << dst_mac << ","
                 << dst_ip << ","
                 << frame_len << "\n";
        log_file.flush();  // 立即写入磁盘
    }
    
    // 在控制台输出（可选，调试用）
    static int packet_count = 0;
    if (++packet_count % 100 == 0) {
        std::cout << "Captured " << packet_count << " packets..." << std::endl;
    }
}

// 统计输出线程函数
void stats_thread_func() {
    while (running) {
        // 等待1分钟
        std::this_thread::sleep_for(std::chrono::minutes(1));
        
        // 加锁获取统计信息
        std::lock_guard<std::mutex> lock(stats_mutex);
        
        // 创建统计文件
        std::string stats_filename = "network_stats_" + getCurrentTime() + ".csv";
        // 替换文件名中的特殊字符
        std::replace(stats_filename.begin(), stats_filename.end(), ':', '-');
        std::replace(stats_filename.begin(), stats_filename.end(), ' ', '_');
        
        std::ofstream stats_file(stats_filename);
        if (!stats_file.is_open()) {
            std::cerr << "Cannot create statistics file!" << std::endl;
            continue;
        }
        
        // 写入统计头部
        stats_file << "Stat Type,Address,Data Length (bytes)\n";
        
        // 写入源MAC统计
        stats_file << "\n=== Source MAC Address Statistics ===\n";
        for (const auto& pair : stats.src_mac_stats) {
            stats_file << "Source MAC," << pair.first << "," << pair.second << "\n";
        }
        
        // 写入目的MAC统计
        stats_file << "\n=== Destination MAC Address Statistics ===\n";
        for (const auto& pair : stats.dst_mac_stats) {
            stats_file << "Destination MAC," << pair.first << "," << pair.second << "\n";
        }
        
        // 写入源IP统计
        stats_file << "\n=== Source IP Address Statistics ===\n";
        for (const auto& pair : stats.src_ip_stats) {
            stats_file << "Source IP," << pair.first << "," << pair.second << "\n";
        }
        
        // 写入目的IP统计
        stats_file << "\n=== Destination IP Address Statistics ===\n";
        for (const auto& pair : stats.dst_ip_stats) {
            stats_file << "Destination IP," << pair.first << "," << pair.second << "\n";
        }
        
        stats_file.close();
        std::cout << "Statistics saved to file: " << stats_filename << std::endl;
        
        // 清空统计（开始下一轮统计）
        stats.src_mac_stats.clear();
        stats.dst_mac_stats.clear();
        stats.src_ip_stats.clear();
        stats.dst_ip_stats.clear();
    }
}

// 获取本机MAC地址
std::string getLocalMacAddress() {
    PIP_ADAPTER_INFO adapter_info = NULL;
    ULONG buffer_len = 0;
    
    // 获取所需的缓冲区大小
    if (GetAdaptersInfo(adapter_info, &buffer_len) == ERROR_BUFFER_OVERFLOW) {
        adapter_info = (PIP_ADAPTER_INFO)malloc(buffer_len);
    }
    
    std::string local_mac = "00-00-00-00-00-00";
    
    if (GetAdaptersInfo(adapter_info, &buffer_len) == NO_ERROR) {
        PIP_ADAPTER_INFO adapter = adapter_info;
        while (adapter) {
            if (adapter->Type == MIB_IF_TYPE_ETHERNET) {
                char mac_str[18];
                sprintf_s(mac_str, sizeof(mac_str), "%02X-%02X-%02X-%02X-%02X-%02X",
                    adapter->Address[0], adapter->Address[1], adapter->Address[2],
                    adapter->Address[3], adapter->Address[4], adapter->Address[5]);
                local_mac = std::string(mac_str);
                break;
            }
            adapter = adapter->Next;
        }
    }
    
    if (adapter_info) free(adapter_info);
    return local_mac;
}

int main() {
    std::cout << "=== WinPcap Network Traffic Monitor ===" << std::endl;
    std::cout << "This program will log network traffic and generate statistics every minute" << std::endl;
    
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t* alldevs;
    
    // 获取设备列表
    std::cout << "\nGetting network device list..." << std::endl;
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "Error: " << errbuf << std::endl;
        return 1;
    }
    
    // 显示设备列表
    std::cout << "\nAvailable network devices:" << std::endl;
    int i = 0;
    for (pcap_if_t* d = alldevs; d; d = d->next) {
        std::cout << ++i << ". " << d->name << std::endl;
        if (d->description) {
            std::cout << "   Description: " << d->description << std::endl;
        }
    }
    
    if (i == 0) {
        std::cout << "No network devices found!" << std::endl;
        pcap_freealldevs(alldevs);
        return 1;
    }
    
    // 让用户选择设备
    int choice;
    std::cout << "\nSelect device number to monitor (1-" << i << "): ";
    std::cin >> choice;
    
    if (choice < 1 || choice > i) {
        std::cout << "Invalid selection!" << std::endl;
        pcap_freealldevs(alldevs);
        return 1;
    }
    
    // 获取选中的设备
    pcap_if_t* selected_dev = alldevs;
    for (int j = 1; j < choice; j++) {
        selected_dev = selected_dev->next;
    }
    
    std::cout << "\nSelected device: " << selected_dev->name << std::endl;
    if (selected_dev->description) {
        std::cout << "Description: " << selected_dev->description << std::endl;
    }
    
    // 打开日志文件
    std::string log_filename = "network_traffic_" + getCurrentTime() + ".csv";
    // 替换文件名中的特殊字符
    std::replace(log_filename.begin(), log_filename.end(), ':', '-');
    std::replace(log_filename.begin(), log_filename.end(), ' ', '_');
    
    log_file.open(log_filename);
    if (!log_file.is_open()) {
        std::cerr << "Cannot create log file!" << std::endl;
        pcap_freealldevs(alldevs);
        return 1;
    }
    
    // 写入CSV头部
    log_file << "Time,Source MAC,Source IP,Destination MAC,Destination IP,Frame Length\n";
    std::cout << "Log file: " << log_filename << std::endl;
    
    // 打开设备
    std::cout << "\nOpening device..." << std::endl;
    pcap_t* handle = pcap_open_live(selected_dev->name, 65536, 1, 1000, errbuf);
    if (!handle) {
        std::cerr << "Cannot open device: " << errbuf << std::endl;
        pcap_freealldevs(alldevs);
        log_file.close();
        return 1;
    }
    
    // 检查数据链路层
    if (pcap_datalink(handle) != DLT_EN10MB) {
        std::cerr << "This device does not support Ethernet!" << std::endl;
        pcap_close(handle);
        pcap_freealldevs(alldevs);
        log_file.close();
        return 1;
    }
    
    // 设置过滤器（只捕获IP包）
    struct bpf_program filter;
    std::string filter_exp = "ip";
    if (pcap_compile(handle, &filter, filter_exp.c_str(), 0, 0) == -1) {
        std::cerr << "Cannot compile filter!" << std::endl;
    } else if (pcap_setfilter(handle, &filter) == -1) {
        std::cerr << "Cannot set filter!" << std::endl;
    }
    
    // 获取本机MAC地址
    std::string local_mac = getLocalMacAddress();
    std::cout << "Local MAC address: " << local_mac << std::endl;
    
    // 释放设备列表
    pcap_freealldevs(alldevs);
    
    // 启动统计线程
    std::cout << "\nStarting statistics thread (output every minute)..." << std::endl;
    std::thread stats_thread(stats_thread_func);
    
    // 开始捕获数据包
    std::cout << "\nStarting network traffic capture..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    int packet_count = 0;
    struct pcap_pkthdr header;
    const u_char* packet;
    
    while (running) {
        packet = pcap_next(handle, &header);
        if (packet) {
            packet_handler(nullptr, &header, packet);
            packet_count++;
        }
        
        // 检查键盘输入（用于退出）
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState(0x43) & 0x8000) {
            std::cout << "\nCtrl+C detected, stopping..." << std::endl;
            running = false;
        }
    }
    
    // 停止捕获
    std::cout << "\nStopped capture. Total packets captured: " << packet_count << std::endl;
    
    // 等待统计线程结束
    running = false;
    stats_thread.join();
    
    // 关闭文件
    log_file.close();
    
    // 关闭pcap句柄
    pcap_close(handle);
    
    std::cout << "Program ended. Log file: " << log_filename << std::endl;
    std::cout << "Press Enter to exit...";
    std::cin.ignore();
    std::cin.get();
    
    return 0;
}