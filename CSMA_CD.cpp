#include <iostream>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <vector>
#include <atomic>

// 信道状态
enum ChannelState {
    IDLE,       // 空闲
    BUSY,       // 忙
    COLLISION   // 冲突
};

// 全局信道状态
std::atomic<ChannelState> channel_state(IDLE);
std::mutex channel_mutex;

// 随机数生成器
std::random_device rd;
std::mt19937 gen(rd());

// 节点类
class CSMA_CD_Node {
private:
    int node_id;
    int collision_count;
    int max_collisions = 16;  // 最大冲突次数
    
public:
    CSMA_CD_Node(int id) : node_id(id), collision_count(0) {}
    
    // 二进制指数退避算法
    int binary_exponential_backoff() {
        int k = std::min(collision_count, 10);  // 最大k=10
        int r = std::uniform_int_distribution<>(0, (1 << k) - 1)(gen);
        return r * 512;  // 返回退避时间（单位为微秒）
    }
    
    // 发送帧
    void send_frame() {
        collision_count = 0;
        
        while(collision_count < max_collisions) {
            // 1. 载波侦听
            while(channel_state == BUSY) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            
            // 2. 发送前等待IFG（帧间间隔）
            std::this_thread::sleep_for(std::chrono::microseconds(96));
            
            // 3. 发送数据
            std::cout << "节点" << node_id << ": 开始发送数据..." << std::endl;
            channel_state = BUSY;
            
            // 模拟传输时间
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            // 4. 冲突检测
            if(channel_state == COLLISION) {
                collision_count++;
                std::cout << "节点" << node_id << ": 检测到冲突，第" 
                          << collision_count << "次冲突" << std::endl;
                
                // 发送拥塞信号
                channel_state = IDLE;
                
                // 5. 退避
                int backoff_time = binary_exponential_backoff();
                std::cout << "节点" << node_id << ": 退避" << backoff_time 
                          << "微秒" << std::endl;
                std::this_thread::sleep_for(std::chrono::microseconds(backoff_time));
                
                continue;
            }
            
            // 发送成功
            std::cout << "节点" << node_id << ": 发送成功" << std::endl;
            channel_state = IDLE;
            break;
        }
        
        if(collision_count >= max_collisions) {
            std::cout << "节点" << node_id << ": 超过最大重试次数，发送失败" << std::endl;
        }
    }
    
    // 模拟接收端监听信道
    void listen_channel() {
        while(true) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            // 这里可以添加接收逻辑
        }
    }
};

// 模拟两个节点同时发送数据
void simulate_two_nodes() {
    CSMA_CD_Node node1(1);
    CSMA_CD_Node node2(2);
    
    std::thread t1([&node1]() { 
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        node1.send_frame(); 
    });
    
    std::thread t2([&node2]() { 
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        node2.send_frame(); 
    });
    
    // 监听线程模拟信道状态变化
    std::thread t3([]() {
        int counter = 0;
        while(true) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            counter++;
            
            // 模拟偶然的冲突
            if(counter % 50 == 0 && channel_state == BUSY) {
                channel_state = COLLISION;
                std::cout << "信道: 检测到冲突" << std::endl;
            }
        }
    });
    
    t1.join();
    t2.join();
    t3.detach();
}

int main() {
    std::cout << "=== CSMA/CD 模拟程序 ===" << std::endl;
    std::cout << "信道状态: 0-空闲, 1-忙, 2-冲突" << std::endl;
    std::cout << std::endl;
    
    // 运行模拟
    simulate_two_nodes();
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "模拟结束" << std::endl;
    
    return 0;
}
