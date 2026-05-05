#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

using namespace std;

mutex mtx;
const int WINDOW_SIZE = 4;
int send_base = 0;      // 已确认的最早序列号
int next_seq = 0;       // 下一个待发送序列号
vector<bool> acks(100, false);  // 记录哪些包已确认
vector<int> window;     // 当前窗口内的序列号
random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0.0, 1.0);

void sender() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        
        // 更新窗口
        window.clear();
        for (int i = send_base; i < send_base + WINDOW_SIZE && i < 100; i++) {
            window.push_back(i);
        }
        
        // 发送未确认的包
        for (int seq : window) {
            if (!acks[seq]) {
                cout << "发送方: 发送 seq=" << seq << endl;
                next_seq = max(next_seq, seq + 1);
                
                // 模拟丢包（30%概率）
                if (dis(gen) < 0.3) {
                    cout << "接收方: 丢包 seq=" << seq << endl;
                } else {
                    cout << "接收方: 收到 seq=" << seq << "，发送ACK" << endl;
                    acks[seq] = true;
                }
            }
        }
        
        // 滑动窗口（确认连续的包）
        while (send_base < 100 && acks[send_base]) {
            acks[send_base] = false;
            send_base++;
        }
        
        // 输出窗口状态
        cout << "\n===== 当前窗口状态 =====" << endl;
        
        vector<int> sent_confirmed, sent_unconfirmed, unsent_allowed, not_allowed;
        for (int i = 0; i < 100; i++) {
            if (i < send_base) {
                sent_confirmed.push_back(i);
            } else if (i < send_base + WINDOW_SIZE) {
                if (acks[i]) {
                    sent_unconfirmed.push_back(i);
                } else {
                    unsent_allowed.push_back(i);
                }
            } else {
                not_allowed.push_back(i);
            }
        }
        
        cout << "已发送并确认: ";
        for (int i : sent_confirmed) cout << i << " ";
        cout << endl;
        
        cout << "已发送未确认: ";
        for (int i : sent_unconfirmed) cout << i << " ";
        cout << endl;
        
        cout << "允许发送未发送: ";
        for (int i : unsent_allowed) cout << i << " ";
        cout << endl;
        
        cout << "不允许发送: ";
        for (int i : not_allowed) cout << i << " ";
        cout << endl;
        
        cout << "通知窗口大小: " << WINDOW_SIZE << endl;
        cout << "可用窗口大小: " << WINDOW_SIZE - (next_seq - send_base) << endl;
        cout << "=========================\n" << endl;
        
        lock.unlock();
        this_thread::sleep_for(chrono::seconds(1));
    }
}

int main() {
    thread t(sender);
    t.join();
    return 0;
}
