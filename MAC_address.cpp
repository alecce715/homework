#include <iostream>
#include <cstring>

#define MAC_ADDRESS_LENGTH 6

// 定义MAC地址类型
typedef unsigned char MAC_address[MAC_ADDRESS_LENGTH];

// 定义以太网帧结构体
struct EthernetFrame {
    unsigned char destination[MAC_ADDRESS_LENGTH];
    unsigned char source[MAC_ADDRESS_LENGTH];
    unsigned short type;  // 类型字段
    unsigned char data[1500];  // 数据部分
    unsigned int fcs;  // 帧校验序列
};

// 本机MAC地址（示例）
MAC_address this_mac_address = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E};

// 判断MAC地址是否匹配
int mac_address_match(const struct EthernetFrame *frame) {
    // 检查广播地址（全FF）
    bool is_broadcast = true;
    for(int i = 0; i < MAC_ADDRESS_LENGTH; i++) {
        if(frame->destination[i] != 0xFF) {
            is_broadcast = false;
            break;
        }
    }
    if(is_broadcast) {
        return 1;
    }
    
    // 检查多播地址（最低位为1）
    bool is_multicast = (frame->destination[0] & 0x01) != 0;
    if(is_multicast) {
        return 1;
    }
    
    // 检查本机地址
    bool is_unicast_match = true;
    for(int i = 0; i < MAC_ADDRESS_LENGTH; i++) {
        if(frame->destination[i] != this_mac_address[i]) {
            is_unicast_match = false;
            break;
        }
    }
    if(is_unicast_match) {
        return 1;
    }
    
    return 0;  // 不匹配
}

int main() {
    // 测试示例
    EthernetFrame test_frame;
    
    // 测试广播帧
    memset(test_frame.destination, 0xFF, MAC_ADDRESS_LENGTH);
    std::cout << "广播帧: " << mac_address_match(&test_frame) << std::endl;
    
    // 测试多播帧
    test_frame.destination[0] = 0x01;  // 设置最低位为1
    std::cout << "多播帧: " << mac_address_match(&test_frame) << std::endl;
    
    // 测试本机单播帧
    memcpy(test_frame.destination, this_mac_address, MAC_ADDRESS_LENGTH);
    std::cout << "本机单播帧: " << mac_address_match(&test_frame) << std::endl;
    
    // 测试其他单播帧
    test_frame.destination[0] = 0x00;
    std::cout << "其他单播帧: " << mac_address_match(&test_frame) << std::endl;
    
    return 0;
}
