#include <stdint.h>

// 定义以太网帧头部结构
struct ethernet_frame_header {
    uint8_t destination[6];     // 目的MAC地址
    uint8_t source[6];          // 源MAC地址
    uint16_t type_length;       // 类型/长度字段
} __attribute__((packed));      // 避免结构体对齐

// 完整的以太网帧结构
struct ethernet_frame {
    struct ethernet_frame_header header;  // 帧头部
    uint8_t data[1500];                   // 数据部分（最大1500字节）
    uint32_t fcs;                         // 帧校验序列
} __attribute__((packed));

// 辅助宏定义
#define BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IPV6 0x86DD
