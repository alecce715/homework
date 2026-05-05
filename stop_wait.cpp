#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>

using namespace std;

mutex mtx;
condition_variable cv;
bool ack_received = false;
int current_seq = 0;
bool packet_lost = false;

// 随机数生成器
random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0.0, 1.0);

void sender() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        
        cout << "发送方: 发送数据包 (seq=" << current_seq << ")" << endl;
        
        // 模拟丢包（30%概率）
        if (dis(gen) < 0.3) {
            cout << "接收方: 丢包，未发送ACK" << endl;
            packet_lost = true;
        } else {
            cout << "接收方: 收到数据包，发送ACK" << endl;
            packet_lost = false;
            ack_received = true;
            cv.notify_one();
        }
        
        // 等待ACK或超时
        if (!cv.wait_for(lock, chrono::seconds(1), []{ return ack_received; })) {
            cout << "发送方: 超时，重传数据包" << endl;
            continue;
        }
        
        // 收到ACK，切换序列号
        ack_received = false;
        current_seq ^= 1;  // 0变1，1变0
        
        lock.unlock();
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

int main() {
    thread t(sender);
    t.join();
    return 0;
}
